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
#include "cmn/ControlSD.h"
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
#define GB_ROM_SIZE             1048576

#define ROM_DIR                 "/roms"
#define GB_POKEMON_RED_ROM      ROM_DIR "/pokemon_red.gb"

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

