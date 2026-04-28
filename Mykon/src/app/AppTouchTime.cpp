/****************************************************
 * AppTouchTime.cpp
 * 
 * The TouchTime Application
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "app/AppTouchTime.h"

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
 * TouchTime_setup()
 * 
 * Description: Setup the TouchTime app
 **************************************************/
void TouchTime_setup( )
{
    Arduino_ST7701_RGBPanel * gfx = Display_getGFX();
    gfx->fillScreen( BLACK );
    gfx->setTextSize( 3 );
}

/***************************************************
 * TouchTime_run()
 * 
 * Description: Run the TouchTime Application
 **************************************************/
void TouchTime_run( void * pvParameters )
{
    Serial.println("TouchTime: Application Started ");

    /* Suspend self on startup */
    vTaskSuspend( NULL );

    Arduino_ST7701_RGBPanel * gfx = Display_getGFX();
    Arduino_GFX* canvas = Display_getCanvas();

    uint8_t touch_count = 0;
    TP_Point touches[TOUCH_MAX] = {};

    ntfy_app_t8 tsk_notifs = NTFY_NONE;

    while( 1 )
    {
        /* Check task notifications */
        if ( xTaskNotifyWait( 0, 0, &tsk_notifs, 0 ) == pdTRUE )
        {
            /* Perform app update */
            if ( tsk_notifs == NTFY_SETUP )
            {
                TouchTime_setup();

                /* Wait a little while transition to avoid initial touches */
                vTaskDelay( pdMS_TO_TICKS( 500 ) );
            }

        }

        /* Handle task periodics */
        if ( tsk_notifs == NTFY_PRDC )
        {
            Touch_getTouches( touches, &touch_count );

            if ( touch_count > 0 )
            {
                int16_t radius = 15;
                gfx->fillCircle( touches[ 0 ].x - radius / 2, touches[ 0 ].y - radius / 2, radius, MAGENTA );
            }
            
            vTaskDelay( pdMS_TO_TICKS( 10 ) );
        }
    }
}
