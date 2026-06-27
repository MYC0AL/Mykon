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
static void NotifySubscribedTasks( ntfy_app_t8 message );

/**********************
 * Variables
 **********************/

/**********************
 * Functions
 **********************/
static void NotifySubscribedTasks( ntfy_app_t8 message )
{
    Mykon_Hook_s hooks[ APP_COUNT_TOTAL ];
    GetMykonHooks( hooks );

    for ( int i = 0; i < APP_COUNT_TOTAL; i++ )
    {
        if ( hooks[ i ].app_sbscrptn == APP_IO
             && hooks[ i ].tsk_hndl != nullptr )
        {
            xTaskNotify( hooks[ i ].tsk_hndl, message, eSetValueWithOverwrite );
        }
    }
}

 /***************************************************
 * IO_setup()
 * 
 * Description: Setup the IO application
 **************************************************/
void IO_setup( )
{
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
    Jystck_drctn_t jstk_dir = JYSTK_NONE;
    Jystck_drctn_t prev_jstk_dir = JYSTK_NONE;
    bool btn_pressed = false;
    bool prev_btn_pressed = false;

    while(1)
    {
        JYSTCK_GetDirection( &jstk_dir );

        if ( jstk_dir != prev_jstk_dir )
        {
            switch( jstk_dir )
            {
                case JYSTK_DOWN:
                    Serial.println( "IO: Joystick Down" );
                    NotifySubscribedTasks( NTFY_IO_JYSTCK_DOWN );
                    break;

                case JYSTK_UP:
                    Serial.println( "IO: Joystick Up" );
                    NotifySubscribedTasks( NTFY_IO_JYSTCK_UP );
                    break;

                case JYSTK_LEFT:
                    Serial.println( "IO: Joystick Left" );
                    NotifySubscribedTasks( NTFY_IO_JYSTCK_LEFT );
                    break;

                case JYSTK_RIGHT:
                    Serial.println( "IO: Joystick Right" );
                    NotifySubscribedTasks( NTFY_IO_JYSTCK_RIGHT );
                    break;

                case JYSTK_CENTER:
                    Serial.println( "IO: Joystick Center" );
                    NotifySubscribedTasks( NTFY_IO_JYSTCK_CENTER );
                    break;

                default:
                    break;
            }

            prev_jstk_dir = jstk_dir;
        }

        btn_pressed = false;
        for( int i = MCP_PIN_FIRST; i < MCP_PIN_CNT; i++ )
        {
            bool pressed = false;
            if ( MCP23017_Read( i, &pressed ) == ERR_NONE && pressed )
            {
                btn_pressed = true;
                Serial.printf( "IO: MCP Pin %d is pressed\n", i );
                break;
            }
        }

        if ( btn_pressed && !prev_btn_pressed )
        {
            Serial.println( "IO: Joystick Button Pressed" );
            NotifySubscribedTasks( NTFY_IO_BTN_JYSTCK );
        }
        prev_btn_pressed = btn_pressed;

        /* Add a small delay to prevent busy waiting */
        vTaskDelay( pdMS_TO_TICKS( 100 ) );
    }
}
