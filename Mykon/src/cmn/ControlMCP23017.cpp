/****************************************************
 * ControlButton.cpp
 *
 * Control buttons connected to the MCP23017 over I2C
 *
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "cmn/ControlMCP23017.h"
#include <Adafruit_MCP23X17.h>

/**********************
 * Defines
 **********************/

/**********************
 * Variables
 **********************/
static Adafruit_MCP23X17    mcp;
static bool                 s_mcp23017_initialized = false;

/* List of active MCP pins               */
/* Ensure to update when adding new pins */
static MCP_Pin_t s_actv_mcp_pins[] =
    {
    MCP_PIN_JYSTK_SW
    };

/**********************
 * Procedures
 **********************/
static bool MCP23017_IsPinActive( MCP_Pin_t pin );

/***************************************************
 * MCP23017_Init()
 *
 * @brief Initialize the MCP23017 expander and configure
 *        a given pin as a pulled-up input.
 **************************************************/
mk_err_t MCP23017_Init( )
{
    if ( !s_mcp23017_initialized )
    {
        if ( !mcp.begin_I2C( MCP23_I2C_ADDR ) )
        {
            s_mcp23017_initialized = false;
            return ERR_GNRL;
        }

        s_mcp23017_initialized = true;
    }

    for ( int i = MCP_PIN_FIRST; i < MCP_PIN_CNT; i++ )
    {
        mcp.pinMode( i, INPUT_PULLUP );
    }
    return ERR_NONE;
}

/***************************************************
 * Button_IsConnected()
 *
 * @brief Check whether the MCP23017 expander is responding.
 **************************************************/
bool MCP23017_IsConnected( )
{
    if ( !s_mcp23017_initialized )
    {
        return false;
    }

    return true;
}

/***************************************************
 * MCP23017_Read()
 *
 * @brief Read a button state from the MCP23017 pin.
 *        Returns true when the button is pressed.
 **************************************************/
mk_err_t MCP23017_Read( MCP_Pin_t pin, bool* pressed)
{
    if ( pressed == nullptr )
    {
        return ERR_INVLD_PARAM;
    }

    if ( !MCP23017_IsPinActive( pin ) )
    {
        return ERR_INVLD_PARAM;
    }

    if ( !s_mcp23017_initialized )
    {
        return ERR_GNRL;
    }

    *pressed = ( mcp.digitalRead( pin ) == LOW );
    return ERR_NONE;
}


/***************************************************
 * MCP23017_IsPinActive()
 *
 * @brief Check if a pin is active.
 *        Returns true if the pin is active.
 **************************************************/
static bool MCP23017_IsPinActive( MCP_Pin_t pin )
{
    for ( int i = 0; i < sizeof( s_actv_mcp_pins ) / sizeof( s_actv_mcp_pins[ 0 ] ); i++ )
    {
        if ( s_actv_mcp_pins[ i ] == pin )
        {
            return true;
        }
    }
    return false;
}