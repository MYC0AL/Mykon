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
Arduino_ST7701_RGBPanel * Display_getGFX();
Arduino_GFX * Display_getCanvas();
mk_err_t Display_FillJPEG( const char * file_name );
