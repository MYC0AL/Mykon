/****************************************************
 * AppIO.cpp
 * 
 * The IO Application
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "app/AppIO.h"
#include "cmn/ControlADS1115.h"
#include "cmn/ControlMCP23017.h"

/**********************
 * Defines
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
 /***************************************************
 * IO_setup()
 * 
 * Description: Setup the IO application
 **************************************************/
void IO_setup( )
{
    /* Initialize Touch Driver */
    Touch_Init();
    Serial.println("Mykon: Touch Driver Initialized");

    /* Initialize MCP23017 I2C button interface */
    if ( MCP23017_Init( ) != ERR_NONE )
    {
        Serial.printf( "IO: Error configuring MCP23017\n" );
    }

    /* Initialize ADS1115 joystick I2C interface */
    if ( ADS1115_Init( ) != ERR_NONE )
    {
        Serial.println( "IO: Error initializing ADS1115 joystick\n" );
    }

    
}

/***************************************************
 * IO_run()
 * 
 * Description: Run the IO application
 **************************************************/
void IO_run( void * pvParameters )
{
    Serial.println("IO: Application Started ");

    /* Setup task */
    IO_setup();

    /* Local variables*/
    int16_t jstk_y = 0;
    int16_t jstk_x = 0;

    while(1)
    {
        if ( ADS1115_ReadNormalized( ADS1115_JYSTK_X, &jstk_x ) == ERR_NONE
            && ADS1115_ReadNormalized( ADS1115_JYSTK_Y, &jstk_y ) == ERR_NONE )
        {
            Serial.printf( "IO: Joystick X=%d Y=%d\n", jstk_x, jstk_y );
        }
        else
        {
            Serial.println( "IO: Joystick read failed" );
        }

        for( int i = MCP_PIN_FIRST; i < MCP_PIN_CNT; i++ )
        {
            bool pressed = false;
            if ( MCP23017_Read( i, &pressed ) == ERR_NONE && pressed )
            {
                Serial.printf( "IO: MCP Pin %d is pressed\n", i );
            }
        }

        /* Add a small delay to prevent busy waiting */
        vTaskDelay( pdMS_TO_TICKS( 100 ) );
    }
}
