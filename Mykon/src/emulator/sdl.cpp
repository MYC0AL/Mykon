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

static uint8_t *frame_buffer;
static uint8_t *scaled_frame_buffer;

static int button_start, button_select, button_a, button_b, button_down,
    button_up, button_left, button_right;

static volatile bool frame_ready = false;
TaskHandle_t draw_task_handle;

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

static void scale_framebuffer(const uint8_t *src, uint8_t *dst, int src_width,
                              int src_height, int dst_width, int dst_height) {
  for (int y = 0; y < dst_height; ++y) {
    int src_y = (y * src_height) / dst_height;
    for (int x = 0; x < dst_width; ++x) {
      int src_x = (x * src_width) / dst_width;
      dst[y * dst_width + x] = src[src_y * src_width + src_x];
    }
  }
}

void draw_task(void *parameter) {
  uint16_t color_palette[] = {0xffff, (16 << 11) + (32 << 5) + 16,
                              (8 << 11) + (16 << 5) + 8, 0x0000};

  while (true) {
    while (!frame_ready) {
      delay(1);
    }
    frame_ready = false;
    if (!tft || !frame_buffer || !scaled_frame_buffer) {
      continue;
    }

    scale_framebuffer(frame_buffer, scaled_frame_buffer, DRAW_WIDTH, DRAW_HEIGHT,
                      SCALED_DRAW_WIDTH, SCALED_DRAW_HEIGHT);
    tft->drawIndexedBitmap(0, SCREEN_OFFSET_Y, scaled_frame_buffer,
                           color_palette, SCALED_DRAW_WIDTH,
                           SCALED_DRAW_HEIGHT);
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
  if (scaled_frame_buffer == nullptr) {
    scaled_frame_buffer =
        (uint8_t *)calloc(SCALED_DRAW_WIDTH * SCALED_DRAW_HEIGHT, 1);
  }
  tft = Display_getGFX();
  if (!tft || !frame_buffer || !scaled_frame_buffer) {
    return;
  }

  tft->fillScreen(BLACK);
  tft->setTextSize(2);

  // gpio_num_t gpios[] = {_left, _right, _down, _up, _start, _select, _a, _b};
  // for (gpio_num_t pin : gpios) {
  //   gpio_pad_select_gpio(pin);
  //   gpio_set_direction(pin, GPIO_MODE_INPUT);
  //   // uncomment to use builtin pullup resistors
  //   //    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
  // }
  xTaskCreatePinnedToCore(draw_task,  /* Function to implement the task */
                          "drawTask", /* Name of the task */
                          10000,      /* Stack size in words */
                          NULL,       /* Task input parameter */
                          0,          /* Priority of the task */
                          &draw_task_handle, /* Task handle. */
                          0); /* Core where the task should run */
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
  if (scaled_frame_buffer == nullptr) {
    scaled_frame_buffer =
        (uint8_t *)calloc(SCALED_DRAW_WIDTH * SCALED_DRAW_HEIGHT, 1);
  }
  return frame_buffer;
}

void sdl_frame(void) { frame_ready = true; }
