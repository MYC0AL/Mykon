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
#define JOYSTICK_ADS1115_ADDR      0x48
#define JOYSTICK_I2C_SDA_PIN       17
#define JOYSTICK_I2C_SCL_PIN       18
#define JOYSTICK_I2C_FREQUENCY     400000

#define JOYSTICK_CHANNEL_X         0
#define JOYSTICK_CHANNEL_Y         1
#define JOYSTICK_DEFAULT_DEADBAND  50

#define JOYSTICK_MAX_RAW           32767
#define JOYSTICK_MAX_PERCENT       1000

/**********************
 * Types
 **********************/

typedef struct
{
    int16_t x;
    int16_t y;
} JoystickRaw_s;

/**********************
 * Functions
 **********************/
mk_err_t Joystick_Init();
mk_err_t Joystick_IsConnected();
mk_err_t Joystick_ReadRaw(int16_t* x, int16_t* y);
mk_err_t Joystick_ReadVoltage(float* x_volt, float* y_volt);
mk_err_t Joystick_ReadNormalized(int16_t* x, int16_t* y, int16_t deadband = JOYSTICK_DEFAULT_DEADBAND);
