/****************************************************
 * AppMykon.h
 * 
 * The Mykon Application
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

/**********************
 * Defines
 **********************/

/**********************
 * Types 
 **********************/
typedef uint8_t Mykon_state_t;
enum
{
    MYKON_STATE_NONE,
    MYKON_STATE_HOME,
    MYKON_STATE_SELECT,
    MYKON_STATE_APP_RUNNING,
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
void Mykon_setup( );
void Mykon_run( void * pvParameters );

