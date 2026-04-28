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
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    1 /* CS */, 12 /* SCK */, 11 /* SDA */,
    45 /* DE */, 4 /* VSYNC */, 5 /* HSYNC */, 21 /* PCLK */,
    39 /* R0 */, 40 /* R1 */, 41 /* R2 */, 42 /* R3 */, 2 /* R4 */,
    0 /* G0/P22 */, 9 /* G1/P23 */, 14 /* G2/P24 */, 47 /* G3/P25 */, 48 /* G4/P26 */, 3 /* G5 */,
    6 /* B0 */, 7 /* B1 */, 15 /* B2 */, 16 /* B3 */, 8 /* B4 */
);

Arduino_ST7701_RGBPanel *gfx = new Arduino_ST7701_RGBPanel(
    bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */,
    true /* IPS */, 480 /* width */, 480 /* height */,
    st7701_type1_init_operations, sizeof(st7701_type1_init_operations),
    true /* BGR */);

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

Arduino_ST7701_RGBPanel * Display_getGFX()
{
    return gfx;
}

Arduino_GFX * Display_getCanvas()
{
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