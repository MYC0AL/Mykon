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

/* Set the analog resolution */
// analogReadResolution( IO_ANALOG_RSLN );
// analogSetPinAttenuation( IO_PIN_19, ADC_11db );
// analogSetPinAttenuation( IO_PIN_20, ADC_11db );
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
    int jstk_y = 0;
    int jstk_x = 0;

    while(1)
    {
        // jstk_x = analogRead( IO_PIN_19 );
        // jstk_y = analogRead( IO_PIN_20 );

        /* Add a small delay to prevent busy waiting */
        vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }
}
