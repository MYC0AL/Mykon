/****************************************************
 * ControlButton.h
 *
 * Control buttons connected to the MCP23017 over I2C
 *
 ****************************************************/
#pragma once

/**********************
 * Includes
 **********************/
#include <Arduino.h>
#include "cmn/Errors.h"

/**********************
 * Types
 **********************/
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
    MCP_PIN_7,

    MCP_PIN_CNT,
    MCP_PIN_FIRST = MCP_PIN_0,

    /* Pin Mapping */
    MCP_PIN_JYSTK_SW = MCP_PIN_7,
};

/**********************
 * Defines
 **********************/
#define MCP23_I2C_ADDR            0x27

/**********************
 * Functions
 **********************/
mk_err_t MCP23017_Init( );
bool     MCP23017_IsConnected( );
mk_err_t MCP23017_Read( MCP_Pin_t pin, bool* pressed);
