/****************************************************
 * Pong.h
 * 
 * The Pong Application
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
 * Types
 **********************/

/**********************
 * Variables
**********************/

/**********************
 * Functions
 **********************/
void Pong_setup( );
void Pong_run( void * pvParameters );

/**********************
 * Classes
 **********************/
