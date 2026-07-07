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
static ntfy_app_t8 GetJoystickNotification( Jystck_drctn_t direction );
static ntfy_app_t8 GetButtonNotification( MCP_Pin_t pin, bool pressed );

/**********************
 * Variables
 **********************/
static MCP_Pin_t s_prev_pin_states[ MCP_PIN_CNT ] = { false };

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
    Serial.println("IO: Application Started");

    /* Setup task */
    IO_setup();

    /* Local variables*/
    Jystck_drctn_t jstk_dir = JYSTK_NONE;
    Jystck_drctn_t prev_jstk_dir = JYSTK_NONE;

    while(1)
    {
        JYSTCK_GetDirection( &jstk_dir );

        if ( jstk_dir != prev_jstk_dir )
        {
            const ntfy_app_t8 notify = GetJoystickNotification( jstk_dir );
            if ( notify != NTFY_NONE )
            {
                Serial.printf( "IO: Joystick %d\n", static_cast<int>( jstk_dir ) );
                NotifySubscribedTasks( notify );
            }

            prev_jstk_dir = jstk_dir;
        }

        for( int i = MCP_PIN_FIRST; i < MCP_PIN_CNT; i++ )
        {
            bool pressed = false;
            if ( MCP23017_Read( i, &pressed ) == ERR_NONE )
            {
                if ( pressed != s_prev_pin_states[ i ] )
                {
                    s_prev_pin_states[ i ] = pressed;
                    Serial.printf( "IO: MCP Pin %d is %s\n", i, pressed ? "pressed" : "released" );
                    NotifySubscribedTasks( GetButtonNotification( i, pressed ) );
                }
            }
        }

        /* Add a small delay to prevent busy waiting */
        vTaskDelay( pdMS_TO_TICKS( 100 ) );
    }
}

/***************************************************
 * NotifySubscribedTasks()
 * 
 * Description: Notify subscribed tasks of event
 **************************************************/
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
 * GetJoystickNotification()
 * 
 * Description: Get the notification for joystick
 *              events
 **************************************************/
static ntfy_app_t8 GetJoystickNotification( Jystck_drctn_t direction )
{
    switch( direction )
    {
        case JYSTK_DOWN:
            return NTFY_IO_JYSTCK_DOWN;

        case JYSTK_UP:
            return NTFY_IO_JYSTCK_UP;

        case JYSTK_LEFT:
            return NTFY_IO_JYSTCK_LEFT;

        case JYSTK_RIGHT:
            return NTFY_IO_JYSTCK_RIGHT;

        case JYSTK_CENTER:
            return NTFY_IO_JYSTCK_CENTER;

        default:
            return NTFY_NONE;
    }
}

/***************************************************
 * GetButtonNotification()
 * 
 * Description: Get the notification for button
 *              events
 **************************************************/
static ntfy_app_t8 GetButtonNotification( MCP_Pin_t pin, bool pressed )
{
    switch( pin )
    {
        case MCP_PIN_JYSTK_SW:
            return pressed ? NTFY_IO_BTN_JYSTCK : NTFY_IO_BTN_JYSTCK_RELEASE;

        case MCP_PIN_BTN_A:
            return pressed ? NTFY_IO_BTN_A : NTFY_IO_BTN_A_RELEASE;

        case MCP_PIN_BTN_X:
            return pressed ? NTFY_IO_BTN_X : NTFY_IO_BTN_X_RELEASE;

        case MCP_PIN_BTN_Y:
            return pressed ? NTFY_IO_BTN_Y : NTFY_IO_BTN_Y_RELEASE;

        case MCP_PIN_BTN_B:
            return pressed ? NTFY_IO_BTN_B : NTFY_IO_BTN_B_RELEASE;

        case MCP_PIN_BTN_HOME:
            return pressed ? NTFY_IO_BTN_HOME : NTFY_IO_BTN_HOME_RELEASE;

        default:
            return NTFY_NONE;
    }
    return NTFY_NONE;
}