/****************************************************
 * ControlJoystick.h
 * 
 * Control the ADS1115-based joystick over I2C
 *
 ****************************************************/
#pragma once

/**********************
 * Includes
 **********************/
#include <Arduino.h>
#include "cmn/Errors.h"

/**********************
 * Defines
 **********************/
#define ADS1115_ADDR      0x48
#define ADS1115_I2C_SDA_PIN       17
#define ADS1115_I2C_SCL_PIN       18

#define ADS1115_DEFAULT_DEADBAND  50

#define ADS1115_MAX_RAW           26400
#define ADS1115_MAX_PERCENT       1000

/**********************
 * Types
 **********************/
typedef uint8_t ADS1115_Channel_t;
enum
{
    ADS1115_JYSTK_X = 0,
    ADS1115_JYSTK_Y = 3,
    ADS1115_BATT    = 2,
};

/**********************
 * Functions
 **********************/
mk_err_t ADS1115_Init();
mk_err_t ADS1115_IsConnected();
mk_err_t ADS1115_ReadRaw( ADS1115_Channel_t channel, int16_t* value );
mk_err_t ADS1115_ReadNormalized( ADS1115_Channel_t channel, int16_t* value );
