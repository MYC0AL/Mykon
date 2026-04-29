/****************************************************
 * AppAudio.h
 * 
 * The Audio Application
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
#include "cmn/ControlSD.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

/**********************
 * Defines
 **********************/
#define AUDIO_FILENAME_01   "/MoonlightBay.mp3"

/**********************
 * Types
 **********************/
enum
{
    AUDIO_PIN_DOUT = 19,
    AUDIO_PIN_BCLK = 20,
    AUDIO_PIN_LRCK = 46,
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
void Audio_setup( );
void Audio_run( void * pvParameters );

