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

#include "emulator/cpu.h"
#include "emulator/gbrom.h"
#include "emulator/lcd.h"
#include "emulator/mem.h"
#include "emulator/rom.h"
#include "emulator/sdl.h"
#include "emulator/timer.h"

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
void GameBoy_setup( );
void GameBoy_run( void * pvParameters );

