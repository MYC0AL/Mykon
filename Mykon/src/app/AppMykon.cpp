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
static void DrawUI( int * curr_app );
static void DrawHome( void );

/**********************
 * Variables
 **********************/
/* UI button definitions */
BtnGUI_s s_btn_left = {0};
BtnGUI_s s_btn_right = {0};
BtnGUI_s s_btn_start = {0};

Mykon_state_t s_mykon_state = MYKON_STATE_NONE;

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
    s_mykon_state = MYKON_STATE_HOME;
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

    int cur_app = 0;
    uint32_t ignore_ui_until_ms = 0;

    DrawHome( );

    while( 1 )
    {
        QueueHandle_t io_queue = IO_getEventQueue();

        /* Handle IO events ( joystick/buttons ) */
        if ( io_queue != nullptr )
        {
            ntfy_app_t32 io_notif;
            while ( xQueueReceive( io_queue, &io_notif, 0 ) == pdTRUE )
            {
                if ( millis() <= ignore_ui_until_ms )
                {
                    continue;
                }

                switch( io_notif )
                {
                    case NTFY_IO_BTN_HOME:
                        /* Suspend Mykon to allow other flow if desired */
                        vTaskSuspend( NULL );
                        break;

                    case NTFY_IO_JYSTCK_LEFT:
                        if ( s_mykon_state == MYKON_STATE_SELECT )
                            {
                            cur_app = ( cur_app - 1 + APP_COUNT_USER ) % APP_COUNT_USER;
                            DrawUI( &cur_app );
                            ignore_ui_until_ms = millis() + 300;
                            }
                        break;

                    case NTFY_IO_JYSTCK_RIGHT:
                        if ( s_mykon_state == MYKON_STATE_SELECT )
                            {
                            cur_app = ( cur_app + 1 ) % APP_COUNT_USER;
                            DrawUI( &cur_app );
                            ignore_ui_until_ms = millis() + 300;
                            }
                        break;

                    case NTFY_IO_BTN_JYSTCK:
                    case NTFY_IO_BTN_A:
                        if ( s_mykon_state == MYKON_STATE_HOME )
                        {
                            s_mykon_state = MYKON_STATE_SELECT;
                            DrawUI( &cur_app );
                            ignore_ui_until_ms = millis() + 300;
                        }
                        else if ( s_mykon_state == MYKON_STATE_SELECT )
                        {
                            s_mykon_state = MYKON_STATE_APP_RUNNING;
                            ignore_ui_until_ms = millis() + 300;

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

        // /* Handle touch on arrows */
        // TP_Point tp[ TOUCH_MAX ];
        // uint8_t touch_count = 0;
        // Touch_getTouches( tp, &touch_count );
        // if ( waiting_for_touch_release )
        // {
        //     if ( touch_count == 0 )
        //     {
        //         waiting_for_touch_release = false;
        //     }
        //     else
        //     {
        //         touch_count = 0;
        //     }
        // }
        // if ( touch_count )
        // {
        //     if ( on_home )
        //     {
        //         if ( Touch_isBtnTouch( s_btn_start, tp[ 0 ] ) == ERR_NONE )
        //         {
        //             on_home = false;
        //             DrawUI( &cur_app );
        //             waiting_for_touch_release = true;
        //             vTaskDelay( pdMS_TO_TICKS( 200 ) );
        //         }
        //     }
        //     else
        //     {
        //         if ( Touch_isBtnTouch( s_btn_left, tp[ 0 ] ) == ERR_NONE )
        //         {
        //             cur_app = ( cur_app - 1 + APP_COUNT_USER ) % APP_COUNT_USER;
        //             DrawUI( &cur_app );
        //             ignore_ui_until_ms = millis() + 200;
        //         }
        //         else if ( Touch_isBtnTouch( s_btn_right, tp[ 0 ] ) == ERR_NONE )
        //         {
        //             cur_app = ( cur_app + 1 ) % APP_COUNT_USER;
        //             DrawUI( &cur_app );
        //             ignore_ui_until_ms = millis() + 200;
        //         }
        //         else
        //         {
        //             /* If touch is not on arrows, treat as center/start tap */
        //             const int16_t w = gfx->width();
        //             const int16_t h = gfx->height();
        //             const int16_t cx = w/2;
        //             const int16_t cy = h/2;
        //             if ( tp[0].x > cx - 60 && tp[0].x < cx + 60 && tp[0].y > cy - 60 && tp[0].y < cy + 60 )
        //             {
        //                 Mykon_Hook_s hooks[ APP_COUNT_TOTAL ];
        //                 GetMykonHooks( hooks );
        //                 xTaskNotify( hooks[ cur_app ].tsk_hndl, NTFY_SETUP, eSetValueWithOverwrite );
        //                 vTaskResume( hooks[ cur_app ].tsk_hndl );
        //                 vTaskDelay( pdMS_TO_TICKS( 100 ) );
        //                 xTaskNotify( hooks[ cur_app ].tsk_hndl, NTFY_STRT, eSetValueWithOverwrite );
        //                 ignore_ui_until_ms = millis() + 200;
        //                 vTaskDelay( pdMS_TO_TICKS( 200 ) );
        //                 vTaskSuspend( NULL );
        //             }
        //         }
        //     }
        // }
        vTaskDelay( pdMS_TO_TICKS( 50 ) );
    }
}

/***************************************************
 * DrawUI()
 * 
 * Description: Draw the UI
 **************************************************/
static void DrawUI( int * curr_app )
{
    /* Local variables */
    Arduino_RGB_Display * gfx = Display_getGFX();
    const int16_t w = gfx->width();
    const int16_t h = gfx->height();
    const char AppInitial[ APP_COUNT_USER ] = { 'T', 'S', 'O', 'P', 'G' };

    gfx->fillScreen( BLACK );

    /* Left button */
    s_btn_left.w = 80;
    s_btn_left.h = 140;
    s_btn_left.x = 10;
    s_btn_left.y = h/2 - (s_btn_left.h/2);
    s_btn_left.c = WHITE;

    /* Right button */
    s_btn_right = s_btn_left;
    s_btn_right.x = w - s_btn_right.w - 10;

    /* Draw buttons */
    gfx->drawRect( s_btn_left.x, s_btn_left.y, s_btn_left.w, s_btn_left.h, s_btn_left.c );
    gfx->drawRect( s_btn_right.x, s_btn_right.y, s_btn_right.w, s_btn_right.h, s_btn_right.c );

    /* Draw arrows */
    gfx->setTextColor( WHITE );
    gfx->setTextSize( 6 );
    gfx->setCursor( s_btn_left.x + 18, s_btn_left.y + 30 );
    gfx->printf( "<" );
    gfx->setCursor( s_btn_right.x + 18, s_btn_right.y + 30 );
    gfx->printf( ">" );

    /* Draw current app in center */
    gfx->setTextSize( 12 );
    gfx->setTextColor( CYAN );
    const int16_t cx = w/2;
    const int16_t cy = h/2;
    gfx->setCursor( cx - 24, cy - 32 );
    gfx->printf( "%c", AppInitial[ *curr_app ] );
}

/***************************************************
 * DrawHome()
 * 
 * Description: Draw the home screen
 **************************************************/
static void DrawHome( )
{
    /* Local variables */
    Arduino_RGB_Display * gfx = Display_getGFX();
    const int16_t w = gfx->width();
    const int16_t h = gfx->height();
    
    gfx->fillScreen( BLACK );

    /* Title */
    gfx->setTextColor( CYAN );
    gfx->setTextSize( 6 );
    gfx->setCursor( w/2 - 60, 20 );
    gfx->printf( "Mykon" );

    /* Start button (oval/rounded rect) */
    s_btn_start.w = 220;
    s_btn_start.h = 80;
    s_btn_start.x = w/2 - (s_btn_start.w/2);
    s_btn_start.y = h/2 - (s_btn_start.h/2) + 40;
    s_btn_start.c = WHITE;

    gfx->drawRoundRect( s_btn_start.x, s_btn_start.y, s_btn_start.w, s_btn_start.h, 24, s_btn_start.c );
    gfx->setTextColor( WHITE );
    gfx->setTextSize( 4 );
    gfx->setCursor( s_btn_start.x + ( s_btn_start.w/2 - 30 ), s_btn_start.y + ( s_btn_start.h/2 - 12 ) );
    gfx->printf( "START" );
}