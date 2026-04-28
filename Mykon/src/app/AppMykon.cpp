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
 * Mykon_run()
 * 
 * Description: Run the Mykon application
 **************************************************/
void Mykon_run( void * pvParameters )
{
    Serial.println("Mykon: Application Started ");

    // const char * file_name = "/golden.jpg";
    // Display_FillJPEG( file_name );

    while( 1 )
    {
        // Serial.println( "Mykon: Heartbeat" );
        vTaskDelay( 1000 );
    }
}
