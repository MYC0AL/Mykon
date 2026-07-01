#include "sdl.h"

#include <Arduino_GFX_Library.h>

#include "SPI.h"
#include "cmn/ControlDisplay.h"

#define _cs 5     // 3 goes to TFT CS
#define _dc 21    // 4 goes to TFT DC
#define _mosi 23  // 5 goes to TFT MOSI
#define _sclk 18  // 6 goes to TFT SCK/CLK
#define _rst 25   // ESP RST to TFT RESET
#define _miso 19  // Not connected
#define _led 22
//       3.3V     // Goes to TFT LED
//       5v       // Goes to TFT Vcc
//       Gnd      // Goes to TFT Gnd
#define _left GPIO_NUM_4
#define _right GPIO_NUM_26
#define _up GPIO_NUM_17
#define _down GPIO_NUM_27
#define _select GPIO_NUM_15
#define _start GPIO_NUM_35
#define _a GPIO_NUM_33
#define _b GPIO_NUM_32

Arduino_GFX *tft = nullptr;

#define DRAW_HEIGHT 144
#define DRAW_WIDTH 160
#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 480
#define SCALE_X 3
#define SCALE_Y 3
#define SCALED_DRAW_WIDTH (DRAW_WIDTH * SCALE_X)
#define SCALED_DRAW_HEIGHT (DRAW_HEIGHT * SCALE_Y)
#define SCREEN_OFFSET_Y ((SCREEN_HEIGHT - SCALED_DRAW_HEIGHT) / 2)

static uint8_t *frame_buffer;        // live buffer, written continuously by
                                      // the emulator core - NOT safe to read
                                      // more than once per draw pass
static uint8_t *snapshot_buffer;     // stable copy of frame_buffer taken once
                                      // per draw pass; diff/scale/prev all
                                      // read from this, never from frame_buffer
                                      // directly, so they stay consistent
                                      // with each other
static uint8_t *prev_frame_buffer;   // last frame we actually drew, for diffing
static uint8_t *scaled_frame_buffer; // scratch buffer, sized for the full
                                      // scaled frame but only partially
                                      // filled/used on dirty-rect updates

static int button_start, button_select, button_a, button_b, button_down,
    button_up, button_left, button_right;

static volatile bool frame_ready = false;
static bool first_frame = true;
TaskHandle_t draw_task_handle;

TaskHandle_t sdl_get_draw_task_handle(void)
{
  return draw_task_handle;
}

void draw_button(bool value, int x_pos, int y_pos,
                 const char *label = nullptr) {
  if (value) {
    tft->fillCircle(x_pos, y_pos, 7, 0xffff);
  } else {
    tft->fillCircle(x_pos, y_pos, 7, 0x0000);
  }
  if (label) {
    int len = strlen(label);
    tft->setCursor(x_pos-(len/2*8), y_pos+20);
    tft->setTextColor(BLACK);
    tft->printf("%s", label);
  }
}

// Finds the smallest rectangle in source (unscaled) coordinates that
// contains every pixel that differs between `cur` and `prev`.
// Returns false if the two buffers are identical (nothing to redraw).
static bool find_dirty_rect(const uint8_t *cur, const uint8_t *prev,
                            int w, int h,
                            int *out_x0, int *out_y0,
                            int *out_x1, int *out_y1) {
  int x0 = w, x1 = -1, y0 = h, y1 = -1;

  for (int y = 0; y < h; ++y) {
    const uint8_t *cur_row = cur + y * w;
    const uint8_t *prev_row = prev + y * w;

    if (memcmp(cur_row, prev_row, w) == 0) {
      continue; // whole row identical, skip scanning it pixel-by-pixel
    }

    if (y < y0) y0 = y;
    if (y > y1) y1 = y;

    // Only bother scanning for the x-range if this row could still
    // widen our current bounding box.
    int rx0 = -1, rx1 = -1;
    for (int x = 0; x < w; ++x) {
      if (cur_row[x] != prev_row[x]) {
        if (rx0 < 0) rx0 = x;
        rx1 = x;
      }
    }
    if (rx0 < x0) x0 = rx0;
    if (rx1 > x1) x1 = rx1;
  }

  if (y1 < 0) {
    return false; // no differences found
  }

  *out_x0 = x0;
  *out_y0 = y0;
  *out_x1 = x1;
  *out_y1 = y1;
  return true;
}

// Nearest-neighbor upscale of the sub-rectangle [x0,y0]-[x1,y1] of `src`
// (which is DRAW_WIDTH wide) into `dst`, using exact integer SCALE_X/SCALE_Y
// factors. dst is packed tightly at width (x1-x0+1)*SCALE_X.
static void scale_dirty_rect(const uint8_t *src, uint8_t *dst,
                             int x0, int y0, int x1, int y1) {
  const int rw = x1 - x0 + 1;
  const int rh = y1 - y0 + 1;
  const int drw = rw * SCALE_X;

  for (int y = 0; y < rh; ++y) {
    const uint8_t *src_row = src + (y0 + y) * DRAW_WIDTH + x0;
    for (int sy = 0; sy < SCALE_Y; ++sy) {
      uint8_t *dst_row = dst + (y * SCALE_Y + sy) * drw;
      for (int x = 0; x < rw; ++x) {
        uint8_t val = src_row[x];
        uint8_t *dst_px = dst_row + x * SCALE_X;
        for (int sx = 0; sx < SCALE_X; ++sx) {
          dst_px[sx] = val;
        }
      }
    }
  }
}

void draw_task(void *parameter) {
  uint16_t color_palette[] = {0xffff, (16 << 11) + (32 << 5) + 16,
                              (8 << 11) + (16 << 5) + 8, 0x0000};

  while (true) {

    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

    frame_ready = false;
    if (!tft || !frame_buffer || !snapshot_buffer || !scaled_frame_buffer ||
        !prev_frame_buffer) {
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

    if (first_frame) {
      // Nothing to diff against yet - draw the whole screen once.
      x0 = 0;
      y0 = 0;
      x1 = DRAW_WIDTH - 1;
      y1 = DRAW_HEIGHT - 1;
      dirty = true;
      first_frame = false;
    } else {
      dirty = find_dirty_rect(snapshot_buffer, prev_frame_buffer, DRAW_WIDTH,
                              DRAW_HEIGHT, &x0, &y0, &x1, &y1);
    }

    if (dirty) {
      scale_dirty_rect(snapshot_buffer, scaled_frame_buffer, x0, y0, x1, y1);

      const int drw = (x1 - x0 + 1) * SCALE_X;
      const int drh = (y1 - y0 + 1) * SCALE_Y;
      const int dest_x = x0 * SCALE_X;
      const int dest_y = SCREEN_OFFSET_Y + y0 * SCALE_Y;

      tft->drawIndexedBitmap(dest_x, dest_y, scaled_frame_buffer,
                             color_palette, drw, drh);
    }

    // Remember what we just drew so the next frame can diff against it.
    // Swap pointers instead of memcpy-ing - snapshot_buffer already holds
    // exactly what we drew, so it just becomes the new prev_frame_buffer.
    uint8_t *tmp = prev_frame_buffer;
    prev_frame_buffer = snapshot_buffer;
    snapshot_buffer = tmp;

    // draw_button(button_up, 30, SCREEN_HEIGHT / 2 - 15);
    // draw_button(button_left, 15, SCREEN_HEIGHT / 2);
    // draw_button(button_right, 45, SCREEN_HEIGHT / 2);
    // draw_button(button_down, 30, SCREEN_HEIGHT / 2 + 15);

    // draw_button(button_select, 30, SCREEN_HEIGHT / 2 + 70, "select");
    // draw_button(button_start, SCREEN_WIDTH - 70, SCREEN_HEIGHT / 2 + 70,
    //             "start");
    // draw_button(button_a, SCREEN_WIDTH - 45, SCREEN_HEIGHT / 2, "a");
    // draw_button(button_b, SCREEN_WIDTH - 15, SCREEN_HEIGHT / 2 - 15, "b");
  }
}

void sdl_init(void) {
  if (frame_buffer == nullptr) {
    frame_buffer = (uint8_t *)calloc(DRAW_WIDTH * DRAW_HEIGHT, 1);
  }
  if (snapshot_buffer == nullptr) {
    snapshot_buffer = (uint8_t *)calloc(DRAW_WIDTH * DRAW_HEIGHT, 1);
  }
  if (prev_frame_buffer == nullptr) {
    prev_frame_buffer = (uint8_t *)calloc(DRAW_WIDTH * DRAW_HEIGHT, 1);
  }
  if (scaled_frame_buffer == nullptr) {
    scaled_frame_buffer =
        (uint8_t *)calloc(SCALED_DRAW_WIDTH * SCALED_DRAW_HEIGHT, 1);
  }
  tft = Display_getGFX();
  if (!tft || !frame_buffer || !snapshot_buffer || !scaled_frame_buffer ||
      !prev_frame_buffer) {
    return;
  }

  tft->fillScreen(BLACK);
  tft->setTextSize(2);
  first_frame = true; // force a full redraw on the very first frame

  // gpio_num_t gpios[] = {_left, _right, _down, _up, _start, _select, _a, _b};
  // for (gpio_num_t pin : gpios) {
  //   gpio_pad_select_gpio(pin);
  //   gpio_set_direction(pin, GPIO_MODE_INPUT);
  //   // uncomment to use builtin pullup resistors
  //   //    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
  // }
  xTaskCreate(  draw_task,  /* Function to implement the task */
                "drawTask", /* Name of the task */
                10000,      /* Stack size in words */
                NULL,       /* Task input parameter */
                1,          /* Priority of the task */
                &draw_task_handle /* Task handle. */
                ); /* Core where the task should run */
}

int sdl_update(void) {
  button_up = 0; // TODO MJB: Map this to where we get the GPIO status
  button_left = 0;
  button_down = 0;
  button_right = 0;

  button_start = 0;
  button_select = 0;

  button_a = 0;
  button_b = 0;
  sdl_frame();
  return 0;
}

unsigned int sdl_get_buttons(void) {
  unsigned int buttons =
      (button_start * 8) | (button_select * 4) | (button_b * 2) | button_a;
  return buttons;
}

unsigned int sdl_get_directions(void) {
  return (button_down * 8) | (button_up * 4) | (button_left * 2) | button_right;
}

uint8_t *sdl_get_framebuffer(void) {
  if (frame_buffer == nullptr) {
    frame_buffer = (uint8_t *)calloc(DRAW_WIDTH * DRAW_HEIGHT, 1);
  }
  if (snapshot_buffer == nullptr) {
    snapshot_buffer = (uint8_t *)calloc(DRAW_WIDTH * DRAW_HEIGHT, 1);
  }
  if (prev_frame_buffer == nullptr) {
    prev_frame_buffer = (uint8_t *)calloc(DRAW_WIDTH * DRAW_HEIGHT, 1);
  }
  if (scaled_frame_buffer == nullptr) {
    scaled_frame_buffer =
        (uint8_t *)calloc(SCALED_DRAW_WIDTH * SCALED_DRAW_HEIGHT, 1);
  }
  return frame_buffer;
}

void sdl_frame(void) { frame_ready = true; }