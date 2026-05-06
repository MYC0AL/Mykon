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

/**********************
 * Defines
 **********************/
#define IO_ANALOG_RSLN 12  // 0–4095

/**********************
 * Types
 **********************/
typedef uint8_t IO_Pin_t;
enum 
{
    IO_PIN_0  = 0,    /* Used for RGB   */
    IO_PIN_1  = 1,    /* Dont use       */
    IO_PIN_4  = 4,    /* Used for RGB   */
    IO_PIN_5  = 5,    /* Used for RGB   */
    IO_PIN_6  = 6,    /* Used for RGB   */
    IO_PIN_7  = 7,    /* Used for RGB   */
    IO_PIN_15 = 15,   /* Used for RGB   */
    IO_PIN_16 = 16,   /* Used for RGB   */
    IO_PIN_17 = 17,   /* Used for I2C   */
    IO_PIN_18 = 18,   /* Used for I2C   */
    IO_PIN_19 = 19,   /* Used for AUDIO */
    IO_PIN_20 = 20,   /* Used for AUDIO */
    IO_PIN_35 = 35,   /* Dont use       */
    IO_PIN_36 = 36,   /* Dont use       */
    IO_PIN_37 = 37,   /* Dont use       */
    IO_PIN_38 = 38,   /* Used for I2C   */
    IO_PIN_39 = 39    /* Used for RGB   */
};

typedef uint8_t MCP_Pin_t;
enum
{
    MCP_PIN_0,
    MCP_PIN_1,
    MCP_PIN_2,
    MCP_PIN_3,
    MCP_PIN_4,
    MCP_PIN_5,
    MCP_PIN_6,
    MCP_PIN_7
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

