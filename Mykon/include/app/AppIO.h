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

/**********************
 * Defines
 **********************/


/**********************
 * Types
 **********************/
enum 
{
    IO_PIN_6  = 6,
    IO_PIN_7  = 7,
    IO_PIN_19 = 19,
    IO_PIN_20 = 20
};

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

