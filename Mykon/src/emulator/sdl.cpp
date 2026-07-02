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
#include "SPI.h"
#include "cmn/ControlDisplay.h"

/**********************
 * Defines
 **********************/

/**********************
 * Function Prototypes
 **********************/
static bool find_dirty_rect( const uint8_t *cur,const uint8_t *prev,int w, int h,int *out_x0, int *out_y0,int *out_x1, int *out_y1 );
static void scale_dirty_rect( const uint8_t *src,uint8_t *dst,int x0,int y0,int x1,int y1 );
static void draw_task(void *parameter);

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
static uint8_t *scaled_frame_buffer;  // scratch buffer, sized for the full
                                      // scaled frame but only partially
                                      // filled/used on dirty-rect updates

static bool first_frame = true;
TaskHandle_t draw_task_handle;

static Gameboy_Buttons_s s_gb_buttons;
static SemaphoreHandle_t s_gb_buttons_mutex = nullptr;


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
 * find_dirty_rect()
 *
 * Description: Find the smallest rectangle containing all changed pixels
 **************************************************/
static bool find_dirty_rect(
                            const uint8_t *cur,
                            const uint8_t *prev,
                            int w, int h,
                            int *out_x0, int *out_y0,
                            int *out_x1, int *out_y1 
                            )
{
  int x0 = w, x1 = -1, y0 = h, y1 = -1;

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
 * scale_dirty_rect()
 *
 * Description: Upscale the dirty rectangle using nearest-neighbor scaling
 **************************************************/
static void scale_dirty_rect(
                              const uint8_t *src,
                              uint8_t *dst,
                              int x0,
                              int y0,
                              int x1,
                              int y1
                            )
{
  const int rw = x1 - x0 + 1;
  const int rh = y1 - y0 + 1;
  const int drw = rw * SCALE_X;

  for ( int y = 0; y < rh; ++y )
  {
    const uint8_t *src_row = src + ( y0 + y ) * DRAW_WIDTH + x0;
    for ( int sy = 0; sy < SCALE_Y; ++sy )
    {
      uint8_t *dst_row = dst + ( y * SCALE_Y + sy ) * drw;
      for ( int x = 0; x < rw; ++x )
      {
        uint8_t val = src_row[ x ];
        uint8_t *dst_px = dst_row + x * SCALE_X;
        for ( int sx = 0; sx < SCALE_X; ++sx )
        {
          dst_px[ sx ] = val;
        }
      }
    }
  }
}

/***************************************************
 * draw_task()
 *
 * Description: Process and draw the emulator frame buffer on the display
 **************************************************/
static void draw_task(void *parameter)
{
  uint16_t color_palette[] = {0xffff, (16 << 11) + (32 << 5) + 16,
                              (8 << 11) + (16 << 5) + 8, 0x0000};

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
      scale_dirty_rect( snapshot_buffer, scaled_frame_buffer, x0, y0, x1, y1 );

      const int drw = ( x1 - x0 + 1 ) * SCALE_X;
      const int drh = ( y1 - y0 + 1 ) * SCALE_Y;
      const int dest_x = x0 * SCALE_X;
      const int dest_y = SCREEN_OFFSET_Y + y0 * SCALE_Y;

      tft->drawIndexedBitmap( dest_x, dest_y, scaled_frame_buffer,
                             color_palette, drw, drh );
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
  if ( frame_buffer == nullptr )
  {
    frame_buffer = ( uint8_t * )calloc( DRAW_WIDTH * DRAW_HEIGHT, 1 );
  }

  if ( snapshot_buffer == nullptr )
  {
    snapshot_buffer = ( uint8_t * )calloc( DRAW_WIDTH * DRAW_HEIGHT, 1 );
  }

  if (prev_frame_buffer == nullptr)
  {
    prev_frame_buffer = (uint8_t *)calloc(DRAW_WIDTH * DRAW_HEIGHT, 1);
  }

  if (scaled_frame_buffer == nullptr)
  {
    scaled_frame_buffer =
        (uint8_t *)calloc(SCALED_DRAW_WIDTH * SCALED_DRAW_HEIGHT, 1);
  }

  tft = Display_getGFX();
  tft->fillScreen( BLACK );

  if (!tft || !frame_buffer || !snapshot_buffer || !scaled_frame_buffer ||
      !prev_frame_buffer)
  {
    return;
  }

  first_frame = true; // force a full redraw on the very first frame

  xTaskCreate( 
              draw_task,  /* Function to implement the task */
              "drawTask", /* Name of the task */
              10000,      /* Stack size in words */
              NULL,       /* Task input parameter */
              1,          /* Priority of the task */
              &draw_task_handle /* Task handle. */
              );

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

  if ( s_gb_buttons_mutex == nullptr )
  {
    s_gb_buttons_mutex = xSemaphoreCreateMutex();
  }

  if ( s_gb_buttons_mutex != nullptr )
  {
    xSemaphoreTake( s_gb_buttons_mutex, portMAX_DELAY );
    buttons = ( unsigned int )( ( s_gb_buttons.start * 8 ) | ( s_gb_buttons.select * 4 ) | ( s_gb_buttons.b * 2 ) | s_gb_buttons.a );
    xSemaphoreGive( s_gb_buttons_mutex );
  }

  return buttons;
}

/***************************************************
 * sdl_set_buttons()
 *
 * Description: Set the current button states
 **************************************************/
void sdl_set_buttons(const Gameboy_Buttons_s *buttons)
{
  if ( s_gb_buttons_mutex == nullptr )
  {
    s_gb_buttons_mutex = xSemaphoreCreateMutex();
  }

  if ( s_gb_buttons_mutex != nullptr )
  {
    xSemaphoreTake( s_gb_buttons_mutex, portMAX_DELAY );
    s_gb_buttons = *buttons;
    xSemaphoreGive( s_gb_buttons_mutex );
  }
}

/***************************************************
 * sdl_get_directions()
 *
 * Description: Return the current directional state as a bitmask
 **************************************************/
unsigned int sdl_get_directions( void )
{
  unsigned int directions = 0;

  if ( s_gb_buttons_mutex == nullptr )
  {
    s_gb_buttons_mutex = xSemaphoreCreateMutex();
  }

  if ( s_gb_buttons_mutex != nullptr )
  {
    xSemaphoreTake( s_gb_buttons_mutex, portMAX_DELAY );
    directions = ( unsigned int )( (s_gb_buttons.down * 8) | (s_gb_buttons.up * 4) | (s_gb_buttons.left * 2) | s_gb_buttons.right );
    xSemaphoreGive( s_gb_buttons_mutex );
  }

  return directions;
}

/***************************************************
 * sdl_read_buttons()
 *
 * Description: Clear buttons after reading
 **************************************************/
void sdl_read_buttons( void )
{
  if ( s_gb_buttons_mutex == nullptr )
  {
    s_gb_buttons_mutex = xSemaphoreCreateMutex();
  }

  if ( s_gb_buttons_mutex != nullptr )
  {
    xSemaphoreTake( s_gb_buttons_mutex, portMAX_DELAY );
    memset( &s_gb_buttons, 0, sizeof( s_gb_buttons ) );
    xSemaphoreGive( s_gb_buttons_mutex );
  }
}

/***************************************************
 * sdl_get_framebuffer()
 *
 * Description: Return the emulator framebuffer pointer
 **************************************************/
uint8_t *sdl_get_framebuffer( void ) 
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
    prev_frame_buffer = (uint8_t *)calloc(DRAW_WIDTH * DRAW_HEIGHT, 1);
  }

  if ( scaled_frame_buffer == nullptr )
  {
    scaled_frame_buffer =
        ( uint8_t * )calloc( SCALED_DRAW_WIDTH * SCALED_DRAW_HEIGHT, 1 );
  }

  return frame_buffer;
}