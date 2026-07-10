/****************************************************
 * ControlDisplay.h
 * 
 * Control the display
 * 
 ****************************************************/
#pragma once

/**********************
 * Includes
 **********************/
#include <Arduino_GFX_Library.h>
#include "JPEGDEC.h"
#include "cmn/Errors.h"
#include "cmn/DrawJPEG.h"
#include "cmn/ControlTouch.h"

/**********************
 * Defines
 **********************/
#define ESP32_8048S043

#define GFX_BL -1
#define TFT_BL GFX_BL

/* Colors */
#define BLACK       ( 0x0000 )
#define WHITE       ( 0xFFFF )
#define CYAN        ( 0x07FF )
#define RED         ( 0xF800 )
#define MAGENTA     ( 0xF81F )
#define DARKGREY    ( 0x7BEF )
#define GREEN       ( 0x07E0 )
#define LIGHTBLACK  ( 0x2104 )
#define GOLD        ( 0xFEA0 )

/**********************
 * Types
 **********************/

/**********************
 * Variables
 **********************/

/**********************
 * Functions
 **********************/

int jpegDrawCallback(JPEGDRAW *pDraw);
Arduino_RGB_Display * Display_getGFX();
Arduino_GFX * Display_getCanvas();
mk_err_t Display_FillJPEG( const char * file_name );
