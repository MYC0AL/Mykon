/****************************************************
 * ControlJoystick.cpp
 * 
 * Control the ADS1115-based joystick over I2C
 *
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "cmn/ControlADS1115.h"
#include <Adafruit_ADS1X15.h>

/**********************
 * Defines
 **********************/

/**********************
 * Variables
 **********************/
static Adafruit_ADS1115 ads;
static bool ads1115_initialized = false;

/***************************************************
 * ADS1115_Init()
 *
 * @brief Initialize I2C and ADS1115 joystick readings.
 **************************************************/
mk_err_t ADS1115_Init()
{
if ( !ads.begin( ADS1115_ADDR ) )
    {
    ads1115_initialized = false;
    return ERR_GNRL;
    }

/* Configure data rate and gain */
ads.setGain( GAIN_ONE );
ads.setDataRate( RATE_ADS1115_128SPS );

ads1115_initialized = true;
return ERR_NONE;
}

/***************************************************
 * ADS1115_IsConnected()
 *
 * @brief Check whether the ADS1115 is responding.
 **************************************************/
mk_err_t ADS1115_IsConnected()
{
if ( !ads1115_initialized )
    {
    return ERR_GNRL;
    }

int16_t test_read = ads.readADC_SingleEnded( 0 );
return ERR_NONE;
}

/***************************************************
 * ADS1115_ReadRaw()
 *
 * @brief Read raw ADC counts from joystick X and Y channels.
 **************************************************/
mk_err_t ADS1115_ReadRaw( ADS1115_Channel_t channel, int16_t* value )
{
if ( value == nullptr )
    {
    return ERR_INVLD_PARAM;    
    }

if ( !ads1115_initialized )
    {
    return ERR_GNRL;
    }

*value = ads.readADC_SingleEnded( channel );
return ERR_NONE;
}

/***************************************************
 * ADS1115_ReadNormalized()
 *
 * @brief Read ADS1115 values as -1000..1000 with deadzone.
 **************************************************/
mk_err_t ADS1115_ReadNormalized( ADS1115_Channel_t channel, int16_t* value )
{
if ( value == nullptr )
    {
    return ERR_INVLD_PARAM;
    }

int16_t raw_x;
mk_err_t err = ADS1115_ReadRaw( channel, &raw_x );

if ( err != ERR_NONE )
    {
    return err;
    }

*value = map( raw_x, 0, ADS1115_MAX_RAW, -ADS1115_MAX_PERCENT, ADS1115_MAX_PERCENT );

if ( abs( *value ) <= ADS1115_DEFAULT_DEADBAND )
    {
    *value = 0;
    }

return ERR_NONE;
}
