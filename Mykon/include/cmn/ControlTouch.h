/****************************************************
 * ControlTouch.h
 * 
 * Control the TAMC_GT911 Touch Library
 * 
 ****************************************************/
#pragma once

/**********************
 * Includes
 **********************/
#include <TAMC_GT911.h>
#include "cmn/Errors.h"

/**********************
 * Defines
 **********************/
#define I2C_SDA_PIN 17
#define I2C_SCL_PIN 18

#define TOUCH_INT 10
#define TOUCH_RST 38

#define TOUCH_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1 480
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 480
#define TOUCH_MAP_Y2 0
#define TOUCH_MAX 5

/**********************
 * Types
 **********************/
typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t c;

} BtnGUI_s;

/**********************
 * Variables
 **********************/
//extern SemaphoreHandle_t xTouchSemaphore;

/**********************
 * Functions
 **********************/
mk_err_t Touch_Init();
TAMC_GT911* Touch_getDriver( );
mk_err_t Touch_getTouches( TP_Point touches[TOUCH_MAX], uint8_t* touch_count );
mk_err_t Touch_isBtnTouch( BtnGUI_s btn, TP_Point touch );
