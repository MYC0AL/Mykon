/****************************************************
 * ControlJoystick.cpp
 * 
 * Control the ADS1115-based joystick over I2C
 *
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "cmn/ControlJoystick.h"
#include <Adafruit_ADS1X15.h>

/**********************
 * Defines
 **********************/

/**********************
 * Variables
 **********************/
static Adafruit_ADS1115 ads;
static bool joystick_initialized = false;

/***************************************************
 * Joystick_Init()
 *
 * @brief Initialize I2C and ADS1115 joystick readings.
 **************************************************/
mk_err_t Joystick_Init()
{
if ( !ads.begin( JOYSTICK_ADS1115_ADDR ) )
    {
    joystick_initialized = false;
    return ERR_GNRL;
    }

/* Configure for 4.096V range and single-ended mode */
ads.setGain( GAIN_TWOTHIRDS );
ads.setDataRate( RATE_ADS1115_128SPS );

joystick_initialized = true;
return ERR_NONE;
}

/***************************************************
 * Joystick_IsConnected()
 *
 * @brief Check whether the ADS1115 is responding.
 **************************************************/
mk_err_t Joystick_IsConnected()
{
if ( !joystick_initialized )
    {
    return ERR_GNRL;
    }

int16_t test_read = ads.readADC_SingleEnded( 0 );
return ERR_NONE;
}

/***************************************************
 * Joystick_ReadRaw()
 *
 * @brief Read raw ADC counts from joystick X and Y channels.
 **************************************************/
mk_err_t Joystick_ReadRaw( int16_t* x, int16_t* y )
{
if ( x == nullptr || y == nullptr )
    {
    return ERR_INVLD_PARAM;    
    }

if ( !joystick_initialized )
    {
    return ERR_GNRL;
    }

*x = ads.readADC_SingleEnded( JOYSTICK_CHANNEL_X );
*y = ads.readADC_SingleEnded( JOYSTICK_CHANNEL_Y );

return ERR_NONE;
}

/***************************************************
 * Joystick_ReadVoltage()
 *
 * @brief Read joystick axis voltages in volts.
 **************************************************/
mk_err_t Joystick_ReadVoltage( float* x_volt, float* y_volt )
{
if ( x_volt == nullptr || y_volt == nullptr )
    {
    return ERR_INVLD_PARAM;
    }

int16_t raw_x;
int16_t raw_y;
mk_err_t err = Joystick_ReadRaw( &raw_x, &raw_y );

if ( err != ERR_NONE )
    {
    return err;
    }

const float volts_per_count = 4.096f / JOYSTICK_MAX_RAW;
*x_volt = raw_x * volts_per_count;
*y_volt = raw_y * volts_per_count;
return ERR_NONE;
}

/***************************************************
 * Joystick_ReadNormalized()
 *
 * @brief Read joystick values as -1000..1000 with deadzone.
 **************************************************/
mk_err_t Joystick_ReadNormalized(int16_t* x, int16_t* y, int16_t deadband)
{
if ( x == nullptr || y == nullptr )
    {
    return ERR_INVLD_PARAM;
    }

int16_t raw_x;
int16_t raw_y;
mk_err_t err = Joystick_ReadRaw( &raw_x, &raw_y );

if ( err != ERR_NONE )
    {
    return err;
    }

*x = map( raw_x, 0, JOYSTICK_MAX_RAW, -JOYSTICK_MAX_PERCENT, JOYSTICK_MAX_PERCENT );
*y = map( raw_y, 0, JOYSTICK_MAX_RAW, -JOYSTICK_MAX_PERCENT, JOYSTICK_MAX_PERCENT );

if (abs( *x ) <= deadband)
    {
    *x = 0;
    }

if (abs( *y ) <= deadband)
    {
    *y = 0;
    }

return ERR_NONE;
}
