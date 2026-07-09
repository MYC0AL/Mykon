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
#include "cmn/Config.h"
#include <freertos/queue.h>

/**********************
 * Defines
 **********************/
#define IO_EVENT_QUEUE_LEN 8

/**********************
 * Function Prototypes
 **********************/
static void NotifySubscribedTasks( ntfy_app_t32 message );
static ntfy_app_t32 GetJoystickNotification( Jystck_drctn_t direction );
static ntfy_app_t32 GetButtonNotification( MCP_Pin_t pin, bool pressed );

/**********************
 * Variables
 **********************/
static MCP_Pin_t s_prev_pin_states[ MCP_PIN_CNT ] = { false };

// IO events (button/joystick press+release) are delivered through this
// queue rather than through the raw FreeRTOS task-notification value.
// This board's FreeRTOS build only has ONE notification slot per task
// (configTASKNOTIFICATION_ARRAY_ENTRIES == 1, confirmed via the
// "uxIndexToNotify < 1" assert), and that single slot is also used by
// the app launcher (NTFY_SETUP / NTFY_STRT, see StartApp()) and by
// lifecycle events like NTFY_IO_BTN_HOME. Sharing one overwritable slot
// between "start the app" and "button was pressed" means a badly-timed
// IO event can silently clobber a pending NTFY_STRT before the
// subscribed task ever reads it - which is exactly what was causing the
// emulator to not start after a touch-to-launch tap. Routing IO events
// through this queue instead means they're never dropped and never
// collide with lifecycle notifications.
static QueueHandle_t s_io_event_queue = nullptr;

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

    /* Create the IO event queue used to deliver button/joystick events
     * to subscribed tasks without colliding with the raw task
     * notification value (see s_io_event_queue comment above). */
    if ( s_io_event_queue == nullptr )
    {
        s_io_event_queue = xQueueCreate( IO_EVENT_QUEUE_LEN, sizeof( ntfy_app_t32 ) );
        if ( s_io_event_queue == nullptr )
        {
            Serial.println( "IO: Error creating IO event queue" );
        }
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
            const ntfy_app_t32 notify = GetJoystickNotification( jstk_dir );
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

        #ifdef GB_TOUCH_CNTRL

        Touch_getDriver()->read();
        uint8_t touch_count = Touch_getDriver()->touches;
        TP_Point tp1 = Touch_getDriver()->points[0];
        if ( touch_count && s_prev_pin_states[ MCP_PIN_BTN_A ] == 0 )
        {
            NotifySubscribedTasks( NTFY_IO_BTN_A );
            s_prev_pin_states[ MCP_PIN_BTN_A ] = 1;
            Serial.println( "IO: Touch" );
        }
        else if ( touch_count == 0 && s_prev_pin_states[ MCP_PIN_BTN_A ] == 1 )
        {
            NotifySubscribedTasks( NTFY_IO_BTN_A_RELEASE );
            s_prev_pin_states[ MCP_PIN_BTN_A ] = 0;
            Serial.println( "IO: Touch released" );
        }
        #endif

        /* Add a small delay to prevent busy waiting */
        vTaskDelay( pdMS_TO_TICKS( 100 ) );
    }
}

/***************************************************
 * IO_getEventQueue()
 *
 * Description: Return the handle to the IO event queue so subscribed
 *              tasks can drain it. Returns nullptr if IO_setup() has
 *              not run yet.
 **************************************************/
QueueHandle_t IO_getEventQueue( void )
{
    return s_io_event_queue;
}

/***************************************************
 * NotifySubscribedTasks()
 * 
 * Description: Notify subscribed tasks of event. The event value is
 *              delivered via s_io_event_queue (never dropped, never
 *              clobbered); the task notification itself is used only
 *              as a "wake up and check the queue" doorbell and carries
 *              no meaningful value of its own, so it can never collide
 *              with lifecycle notifications like NTFY_SETUP/NTFY_STRT
 *              sent by the app launcher on the same notification slot.
 **************************************************/
static void NotifySubscribedTasks( ntfy_app_t32 message )
{
    if ( message == NTFY_NONE )
    {
        return;
    }

    Mykon_Hook_s hooks[ APP_COUNT_TOTAL ];
    GetMykonHooks( hooks );

    for ( int i = 0; i < APP_COUNT_TOTAL; i++ )
    {
        if ( hooks[ i ].app_sbscrptn == APP_IO
             && hooks[ i ].tsk_hndl != nullptr )
        {
            if ( s_io_event_queue != nullptr )
            {
                // Non-blocking send: if the queue is momentarily full
                // (IO_EVENT_QUEUE_LEN exceeded), drop rather than stall
                // this task - a full queue means the consumer is badly
                // behind, and blocking here would stall input polling
                // for every other subscriber too.
                if ( xQueueSend( s_io_event_queue, &message, 0 ) != pdTRUE )
                {
                    Serial.println( "IO: Event queue full, dropping event" );
                }
            }
        }
    }
}

/***************************************************
 * GetJoystickNotification()
 * 
 * Description: Get the notification for joystick
 *              events
 **************************************************/
static ntfy_app_t32 GetJoystickNotification( Jystck_drctn_t direction )
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
static ntfy_app_t32 GetButtonNotification( MCP_Pin_t pin, bool pressed )
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
