/****************************************************
 * AppPong.cpp
 * 
 * The Pong Application
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "app/AppPong.h"

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
 * Pong_setup()
 * 
 * Description: Setup the Pong app
 **************************************************/
void Pong_setup( )
{
    Arduino_ST7701_RGBPanel * gfx = Display_getGFX();
    gfx->fillScreen( BLACK );
    gfx->setTextSize( 3 );
}

/***************************************************
 * Pong_run()
 * 
 * Description: Run the Pong Application
 **************************************************/
void Pong_run( void * pvParameters )
{
    Serial.println("Pong: Application Started ");

    /* Suspend self on startup */
    vTaskSuspend( NULL );

    Arduino_ST7701_RGBPanel * gfx = Display_getGFX();
    Arduino_GFX* canvas = Display_getCanvas();

    uint8_t touch_count = 0;
    TP_Point touches[TOUCH_MAX] = {};

    bool app_started = false;

    ntfy_app_t8 tsk_notifs = NTFY_NONE;

    while( 1 )
    {
        /* Check task notifications */
        if ( xTaskNotifyWait( 0, 0, &tsk_notifs, 0 ) == pdTRUE )
        {
            switch( tsk_notifs )
            {
                case NTFY_SETUP:
                    Pong_setup();
                    vTaskDelay( pdMS_TO_TICKS( 500 ) );
                    break;

                case NTFY_STRT:
                    app_started = true;
                    break;
            }
        }

        /* Handle task periodics */
        if ( app_started )
        {
        
        }
        vTaskDelay( pdMS_TO_TICKS( 10 ) );
    }
}
