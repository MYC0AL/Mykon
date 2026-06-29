/****************************************************
 * Pong.h
 * 
 * The Pong Application
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

/**********************
 * Defines
 **********************/
#define PONG_SCREEN_W        ( 480 )
#define PONG_SCREEN_H        ( 480 )
#define PONG_COURT_INSET     ( 8 )
#define PONG_PADDLE_W        ( 10 )
#define PONG_PADDLE_H        ( 72 )
#define PONG_BALL_SIZE       ( 8 )
#define PONG_PADDLE_OFFSET   ( 18 )
#define PONG_OPPONENT_SPEED  ( 4 )
#define PONG_PLAYER_SPEED    ( 5 )
#define PONG_BALL_MULT_THRE  ( 4 )

/**********************
 * Types
 **********************/
struct PongBall
{
    int16_t x    = PONG_SCREEN_W / 2;
    int16_t y    = PONG_SCREEN_H / 2;
    int16_t vx   = 3;
    int16_t vy   = 2;
    int16_t inc  = 0;
    int16_t size = PONG_BALL_SIZE;
};

struct PongPaddle
{
    int16_t x = PONG_PADDLE_OFFSET;
    int16_t y = PONG_SCREEN_H / 2 - PONG_PADDLE_H / 2;
    int16_t w = PONG_PADDLE_W;
    int16_t h = PONG_PADDLE_H;
};

/**********************
 * Variables
**********************/

/**********************
 * Functions
 **********************/
void Pong_setup( );
void Pong_run( void * pvParameters );

/**********************
 * Classes
 **********************/
