/****************************************************
 * AppTouchTime.h
 * 
 * The TouchTime Application
 * 
 ****************************************************/
#pragma once

/**********************
 * Includes
 **********************/
#include "cmn/Errors.h"
#include <FreeRTOS.h>
#include "cmn/ControlDisplay.h"
#include "cmn/ControlTouch.h"
#include "knl/TaskSetup.h"

/**********************
 * Defines
 **********************/

/**********************
 * Function Prototypes
 **********************/

/**********************
 * Variables
 **********************/

/**********************
 * Classes
 **********************/

/**********************
 * Functions
 **********************/
void TouchTime_setup( );
void TouchTime_run( void * pvParameters );

