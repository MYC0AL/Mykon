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
#include "cmn/ControlJoystick.h"

/**********************
 * Defines
 **********************/

/**********************
 * Function Prototypes
 **********************/

/**********************
 * Variables
 **********************/
static Adafruit_MCP23X17 mcp;
static Adafruit_ADS1115 ads;

MCP_Pin_t mcp_pins[] = 
{
    MCP_PIN_0,
    MCP_PIN_1,
    MCP_PIN_2,
    MCP_PIN_3,
    MCP_PIN_4,
    MCP_PIN_5,
    MCP_PIN_6,
    MCP_PIN_7
};

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
    /* Initialize MCP23017 */
    if ( !mcp.begin_I2C( 0x27 ) )
    {
        Serial.println("IO: Error initializing MCP23017");
        return;
    }

    /* Set MCP23017 pin directions */
    for( int i = 0; i < sizeof( mcp_pins ) / sizeof( MCP_Pin_t ); i++ )
    {
        mcp.pinMode( mcp_pins[i], INPUT_PULLUP );
    }

    /* Initialize ADS1115 joystick I2C interface */
    if ( Joystick_Init() != ERR_NONE )
        {
        }

    if ( !ads.begin( JOYSTICK_ADS1115_ADDR ) )
    {
        Serial.println("IO: Error initializing joystick ADS1115");
    }
    else
    {
        /* Configure for 4.096V range and single-ended mode */
        ads.setGain( GAIN_ONE );
        ads.setDataRate( RATE_ADS1115_128SPS );
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
    int16_t jstk_y = 0;
    int16_t jstk_x = 0;

    while(1)
    {
        // if ( Joystick_ReadRaw( &jstk_x, &jstk_y ) == ERR_NONE )
        // {
        //     Serial.printf( "IO: Joystick X=%d Y=%d\n", jstk_x, jstk_y );
        // }
        // else
        // {
        //     //Serial.println( "IO: Joystick read failed" );
        // }

        int a0 = ads.readADC_SingleEnded(0);
        int a3 = ads.readADC_SingleEnded(3);

        Serial.printf("0:%d 3:%d\n", a0, a3);

        for( int i = 0; i < sizeof( mcp_pins ) / sizeof( MCP_Pin_t ); i++ )
        {
            int state = mcp.digitalRead( mcp_pins[ i ] );
            if ( state == LOW )
            {
                Serial.printf( "IO: MCP Pin %d is LOW\n", mcp_pins[ i ] );
            }
        }

        /* Add a small delay to prevent busy waiting */
        vTaskDelay( pdMS_TO_TICKS( 10 ) );
    }
}
