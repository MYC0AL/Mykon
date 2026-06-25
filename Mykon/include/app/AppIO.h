/****************************************************
 * AppIO.h
 * 
 * The IO Application
 * 
 ****************************************************/
#pragma once

/**********************
 * Includes
 **********************/
#include "cmn/Errors.h"
#include <FreeRTOS.h>
#include "knl/TaskSetup.h"
#include "Arduino.h"
#include <Adafruit_MCP23X17.h>
#include <Adafruit_ADS1X15.h>

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

/**********************
 * Functions
 **********************/
void IO_setup( );
void IO_run( void * pvParameters );

