/****************************************************
 * AppDev.cpp
 * 
 * The Dev Application (for developers)
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "app/AppDev.h"
#include "cmn/DrawJPEG.h"
#include "cmn/ControlTouch.h"
#include "knl/TaskSetup.h"

/**********************
 * Defines
 **********************/

/**********************
 * Function Prototypes
 **********************/
static void ShowAllApps( );
static void StartApp( app_list_t8 app_idx );
static void StopApps( );

/**********************
 * Variables
 **********************/
BtnGUI_s DevAppBtns[ APP_COUNT_USER ];

static char DevAppInitial[ ] = { 'T', 'S', 'O' };
static_assert( sizeof( DevAppInitial ) == APP_COUNT_USER, "Missing Dev char identifier" );

static bool DevEnabled = true;

/**********************
 * Functions
 **********************/

/***************************************************
 * Dev_run()
 * 
 * Description: Run the Mykon application
 **************************************************/
void Dev_run( void * pvParameters )
{
    Serial.println("Dev: Application Started ");

    ShowAllApps();

    while( 1 )
    {
        TP_Point tp[ TOUCH_MAX ];
        uint8_t touch_count = 0;

        Touch_getTouches( tp, &touch_count );

        if ( DevEnabled )
        {
            if ( touch_count )
            {
                for ( int i = 0; i < APP_COUNT_USER; i ++ )
                {
                    if ( Touch_isBtnTouch( DevAppBtns[ i ], tp[ 0 ] ) == ERR_NONE )
                    {
                        /* Stop all apps */
                        StopApps();

                        /* Start desired app */
                        StartApp( i );
                        
                        DevEnabled = false;
                    }
                }
            }
        }
        else if ( touch_count == TOUCH_MAX )
        {
            StopApps();
            ShowAllApps();
            DevEnabled = true;
            vTaskDelay( pdMS_TO_TICKS( 1000 ) );
        }

        /* Delay to allow other tasks to run */
        vTaskDelay( pdMS_TO_TICKS( 50 ) );
    }
}

/***************************************************
 * ShowAllApps()
 * 
 * Description: Draw all applications
 **************************************************/
static void ShowAllApps( )
{
    Arduino_ST7701_RGBPanel * gfx = Display_getGFX();
    gfx->fillScreen( BLACK );

    gfx->setTextColor( GOLD );
    gfx->setTextSize( 4 );
    gfx->setCursor( 0, 20 );
    gfx->printf( "====[ Dev Mode ]====" );
    gfx->setCursor( 0, gfx->height()-30 );
    gfx->printf( "====================" );

    /* Init the app buttons */
    memset( DevAppBtns, 0, sizeof( BtnGUI_s ) * APP_COUNT_USER );
    for ( int i = 0; i < APP_COUNT_USER; i++ )
    {
        DevAppBtns[ i ].x = (i % 3) * 140 + 50;
        DevAppBtns[ i ].y = (i / 3) * 140 + 100;
        DevAppBtns[ i ].w = 110;
        DevAppBtns[ i ].h = 110;
        DevAppBtns[ i ].c = GOLD;
    }

    /* Draw the buttons on the screen */
    for ( int i = 0; i < APP_COUNT_USER; i++ )
    {
        gfx->drawRect( DevAppBtns[ i ].x, DevAppBtns[ i ].y, DevAppBtns[ i ].w, DevAppBtns[ i ].h, DevAppBtns[ i ].c );
        gfx->setCursor( DevAppBtns[ i ].x + 45, DevAppBtns[ i ].y + 35 );
        gfx->printf( "%c", DevAppInitial[ i ] );
    }
}

/***************************************************
 * StartApp()
 * 
 * Description: Start desired App
 **************************************************/
static void StartApp( app_list_t8 app_idx )
{
    /* Get a handle to each app */
    Mykon_Hook_s hooks[ APP_COUNT_TOTAL ];
    GetMykonHooks( hooks );

    /* Notify then resume desired app */
    xTaskNotify( hooks[ app_idx ].tsk_hndl, NTFY_SETUP, eSetValueWithOverwrite);
    vTaskResume( hooks[ app_idx ].tsk_hndl );
    vTaskDelay( pdTICKS_TO_MS( 100 ) );
    xTaskNotify( hooks[ app_idx ].tsk_hndl, NTFY_PRDC, eSetValueWithOverwrite);
}

/***************************************************
 * StartApp()
 * 
 * Description: Start desired App
 **************************************************/
static void StopApps( )
{
    /* Get a handle to each app */
    Mykon_Hook_s hooks[ APP_COUNT_TOTAL ];
    GetMykonHooks( hooks );

    /* Stop all user apps */
    for ( int i = 0; i < APP_COUNT_USER; i++ )
    {
        vTaskSuspend( hooks[ i ].tsk_hndl );
    }
}