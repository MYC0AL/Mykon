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
pinMode( IO_PIN_19, OUTPUT );
Serial.println("IO: pinmode 19 ");

pinMode( IO_PIN_20, OUTPUT );
Serial.println("IO: pinmode 20 ");

digitalWrite( IO_PIN_19, HIGH );
Serial.println("IO: pin 19 HIGH ");

digitalWrite( IO_PIN_20, HIGH );
Serial.println("IO: pin 20 HIGH ");
}

/***************************************************
 * IO_run()
 * 
 * Description: Run the IO application
 **************************************************/
void IO_run( void * pvParameters )
{
    Serial.println("IO: Application Started ");

    while(1)
    {
        ntfy_app_t8 tsk_notifs = NTFY_NONE;

        /* Check task notifications */
        if ( xTaskNotifyWait( 0, 0, &tsk_notifs, 0 ) == pdTRUE )
        {
            /* Perform app update */
            if ( tsk_notifs == NTFY_SETUP )
            {
                Serial.println("IO Setup");
                IO_setup();
            }

        }

        /* Handle task periodics */
        if ( tsk_notifs == NTFY_PRDC )
        {
            vTaskDelay( pdMS_TO_TICKS( 10 ) );
        }

        /* Add a small delay to prevent busy waiting */
        vTaskDelay( pdMS_TO_TICKS( 10 ) );
    }
}
