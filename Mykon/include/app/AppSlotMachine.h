/****************************************************
 * SlotMachine.h
 * 
 * The Slot Machine Application
 * 
 ****************************************************/
#pragma once

/**********************
 * Includes
 **********************/
#include "cmn/Errors.h"
#include <FreeRTOS.h>
#include "cmn/ControlDisplay.h"
#include "cmn/ControlTouch.h"
#include "knl/TaskSetup.h"
#include "arduino_sprite.h"
#include "sprites/reel.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <random>

/**********************
 * Defines
 **********************/
#define REEL_CNT    3
#define REEL_WIDTH  128
#define REEL_HEIGHT 1152
#define SLOT_MID    150
#define ITEM_HEIGHT REEL_WIDTH

/**********************
 * Types
 **********************/
typedef enum
{
    SLOT_STATE_IDLE,
    SLOT_STATE_LOAD,
    SLOT_STATE_SPIN,
    SLOT_STATE_STOP,
    SLOT_STATE_LOCK,
    SLOT_STATE_ITEM,
    SLOT_STATE_COIN,
} SlotState_t;

typedef uint8_t SlotItems_t;
enum 
{
    SLOT_ITEM_SEVEN = 0,
    SLOT_ITEM_BELL,
    SLOT_ITEM_ORANGE,
    SLOT_ITEM_BANANA,
    SLOT_ITEM_BAR,
    SLOT_ITEM_GRAPE,
    SLOT_ITEM_MELON,
    SLOT_ITEM_LEMON,
    SLOT_ITEM_CHERRY,
    SLOT_ITEM_COUNT = SLOT_ITEM_CHERRY + 1,
};

typedef uint8_t SlotWins_t;
enum
{
    SLOT_WIN_NONE = 0,
    SLOT_WIN_SMLL,
    SLOT_WIN_LRGE,
    SLOT_WIN_MNOR,
    SLOT_WIN_MJOR,
    SLOT_WIN_JKPT,
    SLOT_WIN_COUNT = SLOT_WIN_JKPT + 1,
};

typedef struct
{
    Arduino_Sprite* sprite;
    int             scroll;     /* How far the sprite has scrolled */
    int             scroll_sp;  /* How fast the sprite scrolls     */
    uint8_t         scroll_cnt; /* How many times the reels scroll */
    int16_t         scroll_pxl; /* How many pixels the reel needs to move */
    SlotState_t     state;      /* State the reel is in            */
    SlotItems_t     item;       /* Currently selected item         */

} SlotReel_s;

/**********************
 * Variables
**********************/

/**********************
 * Functions
 **********************/
void SlotMachine_setup( );
void SlotMachine_run( void * pvParameters );

/**********************
 * Classes
 **********************/
