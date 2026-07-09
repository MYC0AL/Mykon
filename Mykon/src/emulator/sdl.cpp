/****************************************************
 * sdl.cpp
 * 
 * The SDL/graphics driver
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "sdl.h"
#include <Arduino_GFX_Library.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_heap_caps.h>
#include "SPI.h"
#include "cmn/ControlDisplay.h"

/**********************
 * Defines
 **********************/

// Which core the draw/flush task should run on. This should be the core
// that is NOT running your emulation loop, so a slow full-screen flush
// never stalls emulation (and vice versa). Adjust to match your project -
// if your emulator core loop runs on APP_CPU (1), keep this at PRO_CPU (0).
#ifndef SDL_DRAW_TASK_CORE
#define SDL_DRAW_TASK_CORE 0
#endif

/**********************
 * Function Prototypes
 **********************/
static bool IRAM_ATTR find_dirty_rect( const uint8_t *cur,const uint8_t *prev,int w, int h,int *out_x0, int *out_y0,int *out_x1, int *out_y1 );
static void IRAM_ATTR convert_and_scale_dirty_rect( const uint8_t *src,uint16_t *dst,const uint16_t *palette,int x0,int y0,int x1,int y1 );
static void IRAM_ATTR draw_task(void *parameter);
static void sdl_ensure_mutex( void );
static void sdl_ensure_buffers( void );

/**********************
 * Variables
 **********************/
static Arduino_GFX *tft = nullptr;

static uint8_t *frame_buffer;         // live buffer, written continuously by
                                      // the emulator core - NOT safe to read
                                      // more than once per draw pass
static uint8_t *snapshot_buffer;      // stable copy of frame_buffer taken once
                                      // per draw pass; diff/scale/prev all
                                      // read from this, never from frame_buffer
                                      // directly, so they stay consistent
                                      // with each other
static uint8_t *prev_frame_buffer;    // last frame we actually drew, for diffing

// Scratch buffer for the scaled, ALREADY-COLOR-CONVERTED (RGB565) frame.
// Sized for the full scaled frame but only partially filled/used on
// dirty-rect updates. Storing RGB565 here (instead of palette indices)
// means the color conversion happens exactly once per pixel, and the
// display push can go out via draw16bitRGBBitmap without the library
// having to do its own per-pixel palette lookup at transfer time.
// Allocated DMA-capable so the underlying bus driver can push it via DMA
// where supported.
static uint16_t *scaled_frame_buffer;

static bool first_frame = true;
TaskHandle_t draw_task_handle;

static Gameboy_Buttons_s s_gb_buttons;
static SemaphoreHandle_t s_gb_buttons_mutex = nullptr;

// Guards one-time creation of s_gb_buttons_mutex and one-time allocation
// of the frame buffers below. A portMUX spinlock is safe to use even
// before the scheduler is fully spun up and works correctly across both
// cores, unlike the old "if (ptr == nullptr) ptr = create()" pattern,
// which is a classic check-then-act race if two tasks hit it at once
// (e.g. sdl_init() and sdl_get_framebuffer() called from different
// tasks around the same time).
static portMUX_TYPE s_init_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool s_buffers_ready = false;


/**********************
 * Functions
 **********************/

/***************************************************
 * sdl_get_draw_task_handle()
 *
 * Description: Return the draw task handle
 **************************************************/
TaskHandle_t sdl_get_draw_task_handle( void )
{
  return draw_task_handle;
}

/***************************************************
 * sdl_ensure_mutex()
 *
 * Description: Thread-safe one-time creation of s_gb_buttons_mutex.
 *              Safe to call from any task, any number of times.
 **************************************************/
static void sdl_ensure_mutex( void )
{
  if ( s_gb_buttons_mutex != nullptr )
  {
    return; // fast path, no locking needed once it exists
  }

  portENTER_CRITICAL( &s_init_mux );
  if ( s_gb_buttons_mutex == nullptr )
  {
    s_gb_buttons_mutex = xSemaphoreCreateMutex();
  }
  portEXIT_CRITICAL( &s_init_mux );
}

/***************************************************
 * sdl_ensure_buffers()
 *
 * Description: Thread-safe one-time allocation of all frame buffers.
 *              Safe to call from any task, any number of times.
 **************************************************/
static void sdl_ensure_buffers( void )
{
  if ( s_buffers_ready )
  {
    return; // fast path
  }

  portENTER_CRITICAL( &s_init_mux );
  if ( !s_buffers_ready )
  {
    if ( frame_buffer == nullptr )
    {
      frame_buffer = ( uint8_t * )calloc( DRAW_WIDTH * DRAW_HEIGHT, 1 );
    }

    if ( snapshot_buffer == nullptr )
    {
      snapshot_buffer = ( uint8_t * )calloc( DRAW_WIDTH * DRAW_HEIGHT, 1 );
    }

    if ( prev_frame_buffer == nullptr )
    {
      prev_frame_buffer = ( uint8_t * )calloc( DRAW_WIDTH * DRAW_HEIGHT, 1 );
    }

    if ( scaled_frame_buffer == nullptr )
    {
      // DMA-capable allocation so the display bus driver can push this
      // buffer via DMA where supported, instead of a blocking CPU copy.
      scaled_frame_buffer = ( uint16_t * )heap_caps_calloc(
          SCALED_DRAW_WIDTH * SCALED_DRAW_HEIGHT, sizeof( uint16_t ),
          MALLOC_CAP_DMA | MALLOC_CAP_8BIT );

      if ( scaled_frame_buffer == nullptr )
      {
        // Fall back to regular heap if DMA-capable memory isn't
        // available (e.g. fragmented heap) - transfers will still work,
        // just without the DMA-eligibility guarantee.
        scaled_frame_buffer = ( uint16_t * )calloc(
            SCALED_DRAW_WIDTH * SCALED_DRAW_HEIGHT, sizeof( uint16_t ) );
      }
    }

    s_buffers_ready = ( frame_buffer && snapshot_buffer &&
                         prev_frame_buffer && scaled_frame_buffer );
  }
  portEXIT_CRITICAL( &s_init_mux );
}

/***************************************************
 * find_dirty_rect()
 *
 * Description: Find the smallest rectangle containing all changed pixels
 **************************************************/
static bool IRAM_ATTR find_dirty_rect(
                            const uint8_t *cur,
                            const uint8_t *prev,
                            int w, int h,
                            int *out_x0, int *out_y0,
                            int *out_x1, int *out_y1 
                            )
{
  int x0 = w, x1 = -1, y0 = h, y1 = -1;

  // Once the bounding box has widened to the full row (x0==0 && x1==w-1),
  // no later row can widen it any further. On a full-screen update this
  // triggers almost immediately (first dirty row already spans 0..w-1),
  // which lets us skip the expensive per-pixel x-scan on every remaining
  // dirty row - we only need memcmp (already done above) to know the row
  // differs, plus updating y1. This is the main fix for "whole screen
  // changed" performance, since previously every single row paid the full
  // per-pixel scan cost even though the x-range could never change again.
  bool x_maxed = false;

  for ( int y = 0; y < h; ++y )
    {
    const uint8_t *cur_row = cur + y * w;
    const uint8_t *prev_row = prev + y * w;

    if (memcmp( cur_row, prev_row, w ) == 0)
    { 
      continue; // whole row identical, skip scanning it pixel-by-pixel
    }

    if ( y < y0 ) y0 = y;
    if ( y > y1 ) y1 = y;

    if ( x_maxed )
    {
      // x-range already spans the full width - nothing left to learn
      // from scanning this row pixel-by-pixel.
      continue;
    }

    // Only bother scanning for the x-range if this row could still
    // widen our current bounding box.
    int rx0 = -1, rx1 = -1;
    for ( int x = 0; x < w; ++x )
    {
      if ( cur_row[ x ] != prev_row[ x ] )
      {
        if ( rx0 < 0 ) rx0 = x;
        rx1 = x;
      }
    }

    if ( rx0 < x0 ) x0 = rx0;
    if ( rx1 > x1 ) x1 = rx1;

    if ( x0 == 0 && x1 == w - 1 )
    {
      x_maxed = true;
    }
  }

  if ( y1 < 0 )
  {
    return false; // no differences found
  }

  *out_x0 = x0;
  *out_y0 = y0;
  *out_x1 = x1;
  *out_y1 = y1;
  return true;
}

/***************************************************
 * convert_and_scale_dirty_rect()
 *
 * Description: Convert the palette-indexed dirty rect to RGB565 and
 *              upscale it using nearest-neighbor scaling.
 *
 *              Unlike the previous approach (which recomputed the
 *              palette lookup + horizontal expansion once per
 *              vertically-duplicated scanline), this builds each
 *              horizontally-scaled row ONCE and then memcpy's it to
 *              fill the SCALE_Y duplicate rows. For SCALE_Y > 1 this
 *              cuts the per-pixel conversion work by roughly SCALE_Y
 *              times, which is where most of the full-screen-update
 *              cost was going.
 **************************************************/
static void IRAM_ATTR convert_and_scale_dirty_rect(
                              const uint8_t *src,
                              uint16_t *dst,
                              const uint16_t *palette,
                              int x0,
                              int y0,
                              int x1,
                              int y1
                            )
{
  const int rw = x1 - x0 + 1;
  const int rh = y1 - y0 + 1;
  const int drw = rw * SCALE_X;

  // Reused scratch row, sized for the worst case (a full-width row).
  // static -> allocated once, not on the task's stack every call.
  static uint16_t row_line[ DRAW_WIDTH * SCALE_X ];

  for ( int y = 0; y < rh; ++y )
  {
    const uint8_t *src_row = src + ( y0 + y ) * DRAW_WIDTH + x0;

    // Build the horizontally-scaled, color-converted row once.
    for ( int x = 0; x < rw; ++x )
    {
      uint16_t color = palette[ src_row[ x ] ];
      uint16_t *dst_px = row_line + x * SCALE_X;
      for ( int sx = 0; sx < SCALE_X; ++sx )
      {
        dst_px[ sx ] = color;
      }
    }

    // Stamp the finished row out SCALE_Y times via memcpy instead of
    // recomputing it - this is the biggest win for full-screen updates.
    for ( int sy = 0; sy < SCALE_Y; ++sy )
    {
      uint16_t *dst_row = dst + ( ( size_t )( y * SCALE_Y + sy ) ) * drw;
      memcpy( dst_row, row_line, drw * sizeof( uint16_t ) );
    }
  }
}

/***************************************************
 * draw_task()
 *
 * Description: Process and draw the emulator frame buffer on the display
 **************************************************/
static void IRAM_ATTR draw_task(void *parameter)
{
  static const uint16_t color_palette[] = {
      0xffff, ( uint16_t )( ( 16 << 11 ) + ( 32 << 5 ) + 16 ),
      ( uint16_t )( ( 8 << 11 ) + ( 16 << 5 ) + 8 ), 0x0000
  };

  while (true)
  {

    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

    if (!tft || !frame_buffer || !snapshot_buffer || !scaled_frame_buffer ||
        !prev_frame_buffer)
    {
      continue;
    }

    // Take ONE stable copy of the live frame buffer for this whole pass.
    // frame_buffer is written continuously by the emulator core (not gated
    // by frame_ready), so reading it multiple times across this loop body
    // could see different data each time - e.g. diffing against one state
    // but then saving a *later* state into prev_frame_buffer, which would
    // silently swallow whatever changed in between (that pixel would never
    // get drawn, and future diffs would think it's already up to date).
    // Snapshotting once avoids that entirely.
    memcpy(snapshot_buffer, frame_buffer, DRAW_WIDTH * DRAW_HEIGHT);

    int x0, y0, x1, y1;
    bool dirty;

    if ( first_frame )
    {
      // Nothing to diff against yet - draw the whole screen once.
      x0 = 0;
      y0 = 0;
      x1 = DRAW_WIDTH - 1;
      y1 = DRAW_HEIGHT - 1;
      dirty = true;
      first_frame = false;
    }
    else
    {
      dirty = find_dirty_rect( snapshot_buffer, prev_frame_buffer, DRAW_WIDTH,
                              DRAW_HEIGHT, &x0, &y0, &x1, &y1 );
    }

    if ( dirty )
    {
      convert_and_scale_dirty_rect( snapshot_buffer, scaled_frame_buffer,
                                     color_palette, x0, y0, x1, y1 );

      const int drw = ( x1 - x0 + 1 ) * SCALE_X;
      const int drh = ( y1 - y0 + 1 ) * SCALE_Y;
      const int dest_x = x0 * SCALE_X;
      const int dest_y = SCREEN_OFFSET_Y + y0 * SCALE_Y;

      // Pushing pre-converted RGB565 avoids drawIndexedBitmap's own
      // per-pixel palette lookup at transfer time, and lets the bus
      // driver push the buffer via DMA where the underlying
      // Arduino_GFX bus class supports it (the buffer is allocated
      // DMA-capable below).
      tft->draw16bitRGBBitmap( dest_x, dest_y, scaled_frame_buffer, drw, drh );
    }

    // Remember what we just drew so the next frame can diff against it.
    // Swap pointers instead of memcpy-ing - snapshot_buffer already holds
    // exactly what we drew, so it just becomes the new prev_frame_buffer.
    uint8_t *tmp = prev_frame_buffer;
    prev_frame_buffer = snapshot_buffer;
    snapshot_buffer = tmp;

  }
}

/***************************************************
 * sdl_init()
 *
 * Description: Initialize the SDL/graphics buffers and drawing task
 **************************************************/
void sdl_init( void )
{
  sdl_ensure_buffers();

  tft = Display_getGFX();
  tft->fillScreen( BLACK );

  if ( !tft || !s_buffers_ready )
  {
    return;
  }

  first_frame = true; // force a full redraw on the very first frame

  sdl_ensure_mutex();

  // Pinned to a specific core so a slow full-frame flush overlaps with,
  // rather than stalls, the emulation core. Adjust SDL_DRAW_TASK_CORE at
  // the top of this file if your emulation loop runs on a different core.
  xTaskCreatePinnedToCore(
              draw_task,  /* Function to implement the task */
              "drawTask", /* Name of the task */
              10000,      /* Stack size in words */
              NULL,       /* Task input parameter */
              1,          /* Priority of the task */
              &draw_task_handle, /* Task handle. */
              SDL_DRAW_TASK_CORE
              );

  // Safe to touch s_gb_buttons directly here without the mutex - draw_task
  // was just created above and hasn't run yet, so no other task can be
  // concurrently accessing s_gb_buttons at this point in setup.
  memset( &s_gb_buttons, 0, sizeof( s_gb_buttons ) );

}

/***************************************************
 * sdl_get_buttons()
 *
 * Description: Return the current button state as a bitmask
 **************************************************/
unsigned int sdl_get_buttons( void )
{
  unsigned int buttons = 0;

  sdl_ensure_mutex();

  xSemaphoreTake( s_gb_buttons_mutex, portMAX_DELAY );
  buttons = ( unsigned int )( ( s_gb_buttons.start * 8 ) | ( s_gb_buttons.select * 4 ) | ( s_gb_buttons.b * 2 ) | s_gb_buttons.a );
  xSemaphoreGive( s_gb_buttons_mutex );

  return buttons;
}

/***************************************************
 * sdl_set_buttons()
 *
 * Description: Set the current button states
 **************************************************/
void sdl_set_buttons(const Gameboy_Buttons_s *buttons)
{
  sdl_ensure_mutex();

  xSemaphoreTake( s_gb_buttons_mutex, portMAX_DELAY );
  s_gb_buttons = *buttons;
  xSemaphoreGive( s_gb_buttons_mutex );
}

/***************************************************
 * sdl_get_directions()
 *
 * Description: Return the current directional state as a bitmask
 **************************************************/
unsigned int sdl_get_directions( void )
{
  unsigned int directions = 0;

  sdl_ensure_mutex();

  xSemaphoreTake( s_gb_buttons_mutex, portMAX_DELAY );
  directions = ( unsigned int )( (s_gb_buttons.down * 8) | (s_gb_buttons.up * 4) | (s_gb_buttons.left * 2) | s_gb_buttons.right );
  xSemaphoreGive( s_gb_buttons_mutex );

  return directions;
}

/***************************************************
 * sdl_read_buttons()
 *
 * Description: Clear buttons after reading
 **************************************************/
void sdl_read_buttons( void )
{
  sdl_ensure_mutex();

  xSemaphoreTake( s_gb_buttons_mutex, portMAX_DELAY );
  memset( &s_gb_buttons, 0, sizeof( s_gb_buttons ) );
  xSemaphoreGive( s_gb_buttons_mutex );
}

/***************************************************
 * sdl_get_framebuffer()
 *
 * Description: Return the emulator framebuffer pointer
 **************************************************/
uint8_t *sdl_get_framebuffer( void ) 
{
  sdl_ensure_buffers();
  return frame_buffer;
}