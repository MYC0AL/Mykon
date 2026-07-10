/****************************************************
 * ControlDisplay.cpp
 * 
 * Control the display
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "cmn/ControlDisplay.h"

/**********************
 * Defines
 **********************/

/**********************
 * Types
 **********************/

/**********************
 * Function Prototypes
 **********************/

/**********************
 * Variables
 **********************/

// Serial bus used only to send ST7701 init/config commands (3-wire SPI).
// In the older Arduino_GFX API these CS/SCK/SDA pins were baked directly
// into the RGB panel bus class; the newer API separates "the bus that
// streams pixels" (RGB panel, below) from "the bus that configures the
// panel controller" (this SWSPI bus), since those are two genuinely
// different physical interfaces on an RGB-interface ST7701 panel.
Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC */, 1 /* CS */, 12 /* SCK */, 11 /* SDA/MOSI */,
    GFX_NOT_DEFINED /* MISO */);

// RGB parallel bus - carries only the DE/VSYNC/HSYNC/PCLK + data pins now,
// plus the panel timing values that used to be hidden inside the old
// Arduino_ST7701_RGBPanel class's internals.
//
// NOTE: the hsync/vsync porch values below are a starting point pulled
// from a reference board using the same st7701_type1_init_operations
// table at 480x480 - confirm these against your panel/board vendor's
// example code before trusting them. Wrong porch values show up as a
// shifted, torn, or rolling image rather than a compile error, so it'll
// be obvious quickly if they need adjusting.
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    45 /* DE */, 4 /* VSYNC */, 5 /* HSYNC */, 21 /* PCLK */,
    39 /* R0 */, 40 /* R1 */, 41 /* R2 */, 42 /* R3 */, 2 /* R4 */,
    0 /* G0/P22 */, 9 /* G1/P23 */, 14 /* G2/P24 */, 47 /* G3/P25 */, 48 /* G4/P26 */, 3 /* G5 */,
    6 /* B0 */, 7 /* B1 */, 15 /* B2 */, 16 /* B3 */, 8 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */,
    0 /* pclk_active_neg - unchanged default */,
    GFX_NOT_DEFINED /* prefer_speed - keep automatic for now */,
    false /* useBigEndian - unchanged default */,
    0 /* de_idle_high - unchanged default */,
    0 /* pclk_idle_high - unchanged default */,
    4800 /* bounce_buffer_size_px - THE NEW BIT */
);

// Arduino_ST7701_RGBPanel no longer exists - Arduino_RGB_Display is now
// the generic class for all RGB-interface panels, driven by the
// panel-specific init_operations table (st7701_type1_init_operations)
// and the separate command bus (bus) declared above.
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */,
    st7701_type1_init_operations, sizeof(st7701_type1_init_operations));

Arduino_GFX *canvas = new Arduino_Canvas( 440, 256, gfx, 20, 90 );

/**********************
 * Functions
 **********************/

/***************************************************
 * jpegDrawCallback()
 * 
 * @brief Callback function for drawing JPEGs
 **************************************************/
int jpegDrawCallback(JPEGDRAW *pDraw)
{
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

Arduino_RGB_Display * Display_getGFX()
{
    return gfx;
}

Arduino_GFX * Display_getCanvas()
{
    if ( canvas == nullptr )
    {
        return nullptr;
    }

    return canvas;
}

mk_err_t Display_FillJPEG( const char * file_name )
{
    mk_err_t err = DrawJPEG( file_name, jpegDrawCallback, 
                      true /* useBigEndian */,
                      0 /* x */, 0 /* y */, 
                      gfx->width() /* widthLimit */, 
                      gfx->height() /* heightLimit */
                    );
    return err;
}