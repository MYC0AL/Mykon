/****************************************************
 * AppMykon.cpp
 * 
 * The Mykon Application
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "app/AppMykon.h"
#include "cmn/DrawJPEG.h"
#include "app/AppIO.h"
#include "cmn/ControlTouch.h"
#include "knl/TaskSetup.h"

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
 * Mykon_setup()
 * 
 * Description: Setup the Mykon application
 **************************************************/
void Mykon_setup( )
{
    /* Clear screen */
    Display_getGFX()->fillScreen( BLACK );
}

/***************************************************
 * Mykon_run()
 * 
 * Description: Run the Mykon application
 **************************************************/
void Mykon_run( void * pvParameters )
{
    Serial.println("Mykon: Application Started");

    Mykon_setup();

    Arduino_RGB_Display * gfx = Display_getGFX();

    /* UI button definitions */
    BtnGUI_s btn_left = {0};
    BtnGUI_s btn_right = {0};
    BtnGUI_s btn_start = {0};

    const char AppInitial[ APP_COUNT_USER ] = { 'T', 'S', 'O', 'P', 'G' };
    int cur_app = 0;
    bool on_home = true;
    bool waiting_for_touch_release = false;
    TickType_t ignore_touch_until = 0;

    auto draw_ui = [&]() {
        const int16_t w = gfx->width();
        const int16_t h = gfx->height();

        gfx->fillScreen( BLACK );

        /* Left button */
        btn_left.w = 80;
        btn_left.h = 140;
        btn_left.x = 10;
        btn_left.y = h/2 - (btn_left.h/2);
        btn_left.c = WHITE;

        /* Right button */
        btn_right = btn_left;
        btn_right.x = w - btn_right.w - 10;

        /* Draw buttons */
        gfx->drawRect( btn_left.x, btn_left.y, btn_left.w, btn_left.h, btn_left.c );
        gfx->drawRect( btn_right.x, btn_right.y, btn_right.w, btn_right.h, btn_right.c );

        /* Draw arrows */
        gfx->setTextColor( WHITE );
        gfx->setTextSize( 6 );
        gfx->setCursor( btn_left.x + 18, btn_left.y + 30 );
        gfx->printf( "<" );
        gfx->setCursor( btn_right.x + 18, btn_right.y + 30 );
        gfx->printf( ">" );

        /* Draw current app in center */
        gfx->setTextSize( 12 );
        gfx->setTextColor( CYAN );
        const int16_t cx = w/2;
        const int16_t cy = h/2;
        gfx->setCursor( cx - 24, cy - 32 );
        gfx->printf( "%c", AppInitial[ cur_app ] );
    };

    auto draw_home = [&]() {
        const int16_t w = gfx->width();
        const int16_t h = gfx->height();

        gfx->fillScreen( BLACK );

        /* Title */
        gfx->setTextColor( CYAN );
        gfx->setTextSize( 6 );
        gfx->setCursor( w/2 - 60, 20 );
        gfx->printf( "Mykon" );

        /* Start button (oval/rounded rect) */
        btn_start.w = 220;
        btn_start.h = 80;
        btn_start.x = w/2 - (btn_start.w/2);
        btn_start.y = h/2 - (btn_start.h/2) + 40;
        btn_start.c = WHITE;

        gfx->drawRoundRect( btn_start.x, btn_start.y, btn_start.w, btn_start.h, 24, btn_start.c );
        gfx->setTextColor( WHITE );
        gfx->setTextSize( 4 );
        gfx->setCursor( btn_start.x + (btn_start.w/2 - 30), btn_start.y + (btn_start.h/2 - 12) );
        gfx->printf( "Start" );
    };

    /* Initial draw: home screen */
    draw_home();

    while( 1 )
    {
        QueueHandle_t io_queue = IO_getEventQueue();

        /* Handle IO events (joystick/buttons) */
        if ( io_queue != nullptr )
        {
            ntfy_app_t32 io_notif;
            while ( xQueueReceive( io_queue, &io_notif, 0 ) == pdTRUE )
            {
                switch( io_notif )
                {
                    case NTFY_IO_BTN_HOME:
                        /* Suspend Mykon to allow other flow if desired */
                        vTaskSuspend( NULL );
                        break;

                    case NTFY_IO_JYSTCK_LEFT:
                        if ( !on_home )
                        {
                            cur_app = ( cur_app - 1 + APP_COUNT_USER ) % APP_COUNT_USER;
                            draw_ui();
                        }
                        break;

                    case NTFY_IO_JYSTCK_RIGHT:
                        if ( !on_home )
                        {
                            cur_app = ( cur_app + 1 ) % APP_COUNT_USER;
                            draw_ui();
                        }
                        break;

                    case NTFY_IO_JYSTCK_CENTER:
                    case NTFY_IO_BTN_JYSTCK:
                    case NTFY_IO_BTN_A:
                        if ( on_home )
                        {
                            on_home = false;
                            draw_ui();
                            waiting_for_touch_release = true;
                        }
                        else
                        {
                            /* Start selected app */
                            Mykon_Hook_s hooks[ APP_COUNT_TOTAL ];
                            GetMykonHooks( hooks );
                            xTaskNotify( hooks[ cur_app ].tsk_hndl, NTFY_SETUP, eSetValueWithOverwrite );
                            vTaskResume( hooks[ cur_app ].tsk_hndl );
                            vTaskDelay( pdMS_TO_TICKS( 100 ) );
                            xTaskNotify( hooks[ cur_app ].tsk_hndl, NTFY_STRT, eSetValueWithOverwrite );

                            /* Suspend Mykon while selected app runs */
                            vTaskSuspend( NULL );
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        /* Handle touch on arrows */
        TP_Point tp[ TOUCH_MAX ];
        uint8_t touch_count = 0;
        Touch_getTouches( tp, &touch_count );
        if ( waiting_for_touch_release )
        {
            if ( touch_count == 0 )
            {
                waiting_for_touch_release = false;
            }
            else
            {
                touch_count = 0;
            }
        }
        if ( touch_count )
        {
            if ( on_home )
            {
                if ( Touch_isBtnTouch( btn_start, tp[ 0 ] ) == ERR_NONE )
                {
                    on_home = false;
                    draw_ui();
                    waiting_for_touch_release = true;
                    vTaskDelay( pdMS_TO_TICKS( 200 ) );
                }
            }
            else
            {
                if ( Touch_isBtnTouch( btn_left, tp[ 0 ] ) == ERR_NONE )
                {
                    cur_app = ( cur_app - 1 + APP_COUNT_USER ) % APP_COUNT_USER;
                    draw_ui();
                    vTaskDelay( pdMS_TO_TICKS( 200 ) );
                }
                else if ( Touch_isBtnTouch( btn_right, tp[ 0 ] ) == ERR_NONE )
                {
                    cur_app = ( cur_app + 1 ) % APP_COUNT_USER;
                    draw_ui();
                    vTaskDelay( pdMS_TO_TICKS( 200 ) );
                }
                else
                {
                    /* If touch is not on arrows, treat as center/start tap */
                    const int16_t w = gfx->width();
                    const int16_t h = gfx->height();
                    const int16_t cx = w/2;
                    const int16_t cy = h/2;
                    if ( tp[0].x > cx - 60 && tp[0].x < cx + 60 && tp[0].y > cy - 60 && tp[0].y < cy + 60 )
                    {
                        Mykon_Hook_s hooks[ APP_COUNT_TOTAL ];
                        GetMykonHooks( hooks );
                        xTaskNotify( hooks[ cur_app ].tsk_hndl, NTFY_SETUP, eSetValueWithOverwrite );
                        vTaskResume( hooks[ cur_app ].tsk_hndl );
                        vTaskDelay( pdMS_TO_TICKS( 100 ) );
                        xTaskNotify( hooks[ cur_app ].tsk_hndl, NTFY_STRT, eSetValueWithOverwrite );
                        vTaskDelay( pdMS_TO_TICKS( 200 ) );
                        vTaskSuspend( NULL );
                    }
                }
            }
        }
        vTaskDelay( pdMS_TO_TICKS( 50 ) );
    }
}
