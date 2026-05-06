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
#define MCP23017_EXISTS 0

/**********************
 * Function Prototypes
 **********************/

/**********************
 * Variables
 **********************/
Adafruit_MCP23X17 mcp;

#if MCP23017_EXISTS
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
#endif

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
    #if MCP23017_EXISTS
    /* Initialize MCP23017 */
    if ( !mcp.begin_I2C( ) )
    {
        Serial.println("IO: Error initializing MCP23017");
        return;
    }

    /* Set MCP23017 pin directions */
    for( int i = 0; i < sizeof(mcp_pins) / sizeof(MCP_Pin_t); i++ )
    {
        mcp.pinMode( mcp_pins[i], INPUT_PULLUP );
    }
    #endif

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
    int jstk_y = 0;
    int jstk_x = 0;

    while(1)
    {
        // jstk_x = analogRead( IO_PIN_19 );
        // jstk_y = analogRead( IO_PIN_20 );

#if MCP23017_EXISTS
        for( int i = 0; i < sizeof( mcp_pins ) / sizeof( MCP_Pin_t ); i++ )
        {
            int state = mcp.digitalRead( mcp_pins[ i ] );
            Serial.printf("IO: MCP Pin %d State: %d\n", mcp_pins[ i ], state);
        }
#endif

        /* Add a small delay to prevent busy waiting */
        vTaskDelay( pdMS_TO_TICKS( 10 ) );
    }
}
