/****************************************************
 * ControlTouch.cpp
 * 
 * Control the TAMC_GT911 Touch Library
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "cmn/ControlTouch.h"

/**********************
 * Defines
 **********************/

/**********************
 * Types
 **********************/

/**********************
 * Variables
 **********************/
static TAMC_GT911 ts = TAMC_GT911(I2C_SDA_PIN, I2C_SCL_PIN, TOUCH_INT, TOUCH_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

/**********************
 * Functions
 **********************/

mk_err_t Touch_Init()
{
    ts.begin();
    return ERR_NONE;
}

/***************************************************
 * Touch_getDriver()
 * 
 * @brief Get the Touch Driver object.
 **************************************************/
TAMC_GT911* Touch_getDriver( )
{
    return &ts;
}

/***************************************************
 * Touch_getTouches()
 * 
 * @brief Get the current touches.
 **************************************************/
mk_err_t Touch_getTouches( TP_Point touches[TOUCH_MAX], uint8_t* touch_count )
{
    /* Read the TAMC_GT911 for touch information */
    ts.read();

    /* Set the touch count */
    *touch_count = ts.touches;

    for (uint8_t i = 0; i < *touch_count && (i < TOUCH_MAX); ++i)
    {
        touches[i].x = map(ts.points[i].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
        touches[i].y = map(ts.points[i].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
    }

    return ERR_NONE;
}

/***************************************************
 * Touch_isBtnTouch()
 * 
 * @brief Check if a button is being pressed
 **************************************************/
mk_err_t Touch_isBtnTouch( BtnGUI_s btn, TP_Point touch )
{
    mk_err_t ret_val = ERR_GNRL;

    if ( touch.x > btn.x && touch.x < btn.x + btn.w )
        if ( touch.y > btn.y && touch.y < btn.y + btn.h )
            ret_val = ERR_NONE;

    return ret_val;
}