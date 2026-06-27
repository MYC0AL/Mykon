/****************************************************
 * AppPong.cpp
 * 
 * The Pong Application
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "app/AppPong.h"

/**********************
 * Defines
 **********************/
/* Note: A namespace is used here as a replacement to 'static' globals.
        This keeps it local to the file without other files stealing it with 'extern' */
namespace 
{
    PongBall   g_ball;
    PongPaddle g_player_paddle;
    PongPaddle g_opponent_paddle;
}

/**********************
 * Function Prototypes
 **********************/
static int16_t PongClamp( int16_t value, int16_t min_val, int16_t max_val );
static void    PongDrawCenterLine( );
static void    PongDrawCourt( );
static void    PongFillRectClipped( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color );
static void    PongDrawPaddle( const PongPaddle &paddle, uint16_t color );
static void    PongDrawBall( const PongBall &ball, uint16_t color );
static void    PongResetBall( );
static void    PongResetGame( );

/**********************
 * Variables
 **********************/

/**********************
 * Functions
 **********************/

 /***************************************************
 * Pong_setup()
 * 
 * Description: Setup the Pong app
 **************************************************/
void Pong_setup( )
{
    PongResetGame();
    PongDrawCourt();
    PongDrawPaddle( g_player_paddle, CYAN );
    PongDrawPaddle( g_opponent_paddle, RED );
    PongDrawBall( g_ball, WHITE );
}

/***************************************************
 * Pong_run()
 * 
 * Description: Run the Pong Application
 **************************************************/
void Pong_run( void * pvParameters )
{
    Serial.println("Pong: Application Started ");

    /* Suspend self on startup */
    vTaskSuspend( NULL );

    Arduino_ST7701_RGBPanel * gfx = Display_getGFX();

    bool app_started = false;
    bool move_up = false;
    bool move_down = false;

    ntfy_app_t8 tsk_notifs = NTFY_NONE;

    while( 1 )
    {
        /* Check task notifications */
        if ( xTaskNotifyWait( 0, 0, &tsk_notifs, 0 ) == pdTRUE )
        {
            switch( tsk_notifs )
            {
                case NTFY_SETUP:
                    Pong_setup();
                    vTaskDelay( pdMS_TO_TICKS( 500 ) );
                    break;

                case NTFY_STRT:
                    app_started = true;
                    break;

                case NTFY_IO_JYSTCK_UP:
                    move_up = true;
                    move_down = false;
                    break;

                case NTFY_IO_JYSTCK_DOWN:
                    move_up = false;
                    move_down = true;
                    break;

                case NTFY_IO_JYSTCK_CENTER:
                case NTFY_IO_JYSTCK_LEFT:
                case NTFY_IO_JYSTCK_RIGHT:
                    move_up = false;
                    move_down = false;
                    break;
            }
        }

        /* Handle task periodics */
        if ( app_started )
        {
            /* Save previous frame positions so we only redraw changed regions */
            const int16_t prev_player_x = g_player_paddle.x;
            const int16_t prev_player_y = g_player_paddle.y;
            const int16_t prev_opponent_x = g_opponent_paddle.x;
            const int16_t prev_opponent_y = g_opponent_paddle.y;
            const int16_t prev_ball_x = g_ball.x;
            const int16_t prev_ball_y = g_ball.y;

            bool player_redraw = false;
            bool opponent_redraw = false;
            bool ball_redraw = false;

            /* Handle player movement from joystick notifications */
            if ( move_up )
            {
                g_player_paddle.y = PongClamp( g_player_paddle.y - 4, PONG_COURT_INSET, PONG_SCREEN_H - g_player_paddle.h - PONG_COURT_INSET );
            }
            else if ( move_down )
            {
                g_player_paddle.y = PongClamp( g_player_paddle.y + 4, PONG_COURT_INSET, PONG_SCREEN_H - g_player_paddle.h - PONG_COURT_INSET );
            }

            player_redraw = ( g_player_paddle.x != prev_player_x ) || ( g_player_paddle.y != prev_player_y );

            /* Opponent AI */
            if ( g_ball.y + (g_ball.size / 2) > g_opponent_paddle.y + (g_opponent_paddle.h / 2) )
            {
                g_opponent_paddle.y = PongClamp( g_opponent_paddle.y + PONG_OPPONENT_SPEED, PONG_COURT_INSET, PONG_SCREEN_H - g_opponent_paddle.h - PONG_COURT_INSET );
            }
            else if ( g_ball.y + (g_ball.size / 2) < g_opponent_paddle.y + (g_opponent_paddle.h / 2) )
            {
                g_opponent_paddle.y = PongClamp( g_opponent_paddle.y - PONG_OPPONENT_SPEED, PONG_COURT_INSET, PONG_SCREEN_H - g_opponent_paddle.h - PONG_COURT_INSET );
            }

            opponent_redraw = ( g_opponent_paddle.x != prev_opponent_x ) || ( g_opponent_paddle.y != prev_opponent_y );

            /* Update ball position */
            g_ball.x += g_ball.vx;
            g_ball.y += g_ball.vy;
            ball_redraw = ( g_ball.x != prev_ball_x ) || ( g_ball.y != prev_ball_y );

            /* Bounce off top and bottom walls */
            if ( g_ball.y <= PONG_COURT_INSET )
            {
                g_ball.y = PONG_COURT_INSET;
                g_ball.vy = -g_ball.vy;
            }
            else if ( g_ball.y + g_ball.size >= PONG_SCREEN_H - PONG_COURT_INSET )
            {
                g_ball.y = PONG_SCREEN_H - PONG_COURT_INSET - g_ball.size;
                g_ball.vy = -g_ball.vy;
            }

            /* Reset when ball leaves the playfield behind a paddle */
            if ( g_ball.x + g_ball.size < 0 || g_ball.x > PONG_SCREEN_W )
            {
                PongResetBall();
            }

            /* Bounce off paddles and speed up */
            const bool hit_player = ( g_ball.x <= g_player_paddle.x + g_player_paddle.w ) &&
                                     ( g_ball.x + g_ball.size >= g_player_paddle.x ) &&
                                     ( g_ball.y + g_ball.size >= g_player_paddle.y ) &&
                                     ( g_ball.y <= g_player_paddle.y + g_player_paddle.h ) &&
                                     ( g_ball.vx < 0 );

            const bool hit_opponent = ( g_ball.x + g_ball.size >= g_opponent_paddle.x ) &&
                                      ( g_ball.x <= g_opponent_paddle.x + g_opponent_paddle.w ) &&
                                      ( g_ball.y + g_ball.size >= g_opponent_paddle.y ) &&
                                      ( g_ball.y <= g_opponent_paddle.y + g_opponent_paddle.h ) &&
                                      ( g_ball.vx > 0 );

            if ( hit_player || hit_opponent )
            {
                if ( hit_player )
                {
                    g_ball.x = g_player_paddle.x + g_player_paddle.w + 1;
                    g_ball.vx = abs( g_ball.vx );
                }
                else
                {
                    g_ball.x = g_opponent_paddle.x - g_ball.size - 1;
                    g_ball.vx = -abs( g_ball.vx );
                }

                const int16_t paddle_center = ( hit_player ? g_player_paddle.y : g_opponent_paddle.y ) + ( hit_player ? g_player_paddle.h : g_opponent_paddle.h ) / 2;
                const int16_t impact = ( g_ball.y + (g_ball.size / 2) ) - paddle_center;
                g_ball.vy = PongClamp( g_ball.vy + ( impact / 10 ), -6, 6 );
                ball_redraw = true;
            }

            /* Draw only the regions that changed */
            if ( player_redraw )
            {
                PongFillRectClipped( prev_player_x, prev_player_y, g_player_paddle.w, g_player_paddle.h, BLACK );
                PongDrawPaddle( g_player_paddle, CYAN );
            }
            if ( opponent_redraw )
            {
                PongFillRectClipped( prev_opponent_x, prev_opponent_y, g_opponent_paddle.w, g_opponent_paddle.h, BLACK );
                PongDrawPaddle( g_opponent_paddle, RED );
            }
            if ( ball_redraw )
            {
                PongFillRectClipped( prev_ball_x, prev_ball_y, g_ball.size, g_ball.size, BLACK );
                PongDrawBall( g_ball, WHITE );
            }

            PongDrawCenterLine();
        }
        vTaskDelay( pdMS_TO_TICKS( 10 ) );
    }
}

/***************************************************
 * PongClamp()
 * 
 * Description: Clamp a value between a minimum 
 *              and maximum
 **************************************************/
static int16_t PongClamp( int16_t value, int16_t min_val, int16_t max_val )
{
    if ( value < min_val )
    {
        return min_val;
    }
    if ( value > max_val )
    {
        return max_val;
    }
    return value;
}

/***************************************************
 * PongDrawCenterLine()
 * 
 * Description: Draw the center line of the Pong
 *              court
 **************************************************/
static void PongDrawCenterLine( )
{
    Arduino_ST7701_RGBPanel * gfx = Display_getGFX();
    for ( int16_t y = PONG_COURT_INSET + 8; y < PONG_SCREEN_H - PONG_COURT_INSET; y += 20 )
    {
        gfx->fillRect( PONG_SCREEN_W / 2 - 1, y, 2, 10, WHITE );
    }
}

/***************************************************
 * PongDrawCourt()
 * 
 * Description: Draw the Pong court with borders
 *              and center line
 **************************************************/
static void PongDrawCourt( )
{
    Arduino_ST7701_RGBPanel * gfx = Display_getGFX();
    gfx->fillScreen( BLACK );

    gfx->drawRect( PONG_COURT_INSET, PONG_COURT_INSET, PONG_SCREEN_W - (2 * PONG_COURT_INSET), PONG_SCREEN_H - (2 * PONG_COURT_INSET), WHITE );
    PongDrawCenterLine();
}

/***************************************************
 * PongFillRectClipped()
 * 
 * Description: Fill a rectangle on the screen,
 *              clipping it to the court boundaries
 **************************************************/
static void PongFillRectClipped( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color )
{
    const int16_t left = ( x > ( PONG_COURT_INSET + 1 ) ) ? x : ( PONG_COURT_INSET + 1 );
    const int16_t top = ( y > ( PONG_COURT_INSET + 1 ) ) ? y : ( PONG_COURT_INSET + 1 );
    const int16_t right = ( ( x + w ) < ( PONG_SCREEN_W - PONG_COURT_INSET - 1 ) ) ? ( x + w ) : ( PONG_SCREEN_W - PONG_COURT_INSET - 1 );
    const int16_t bottom = ( ( y + h ) < ( PONG_SCREEN_H - PONG_COURT_INSET - 1 ) ) ? ( y + h ) : ( PONG_SCREEN_H - PONG_COURT_INSET - 1 );

    if ( left < right && top < bottom )
    {
        Display_getGFX()->fillRect( left, top, right - left, bottom - top, color );
    }
}

/***************************************************
 * PongDrawPaddle()
 * 
 * Description: Draw a paddle on the screen
 **************************************************/
static void PongDrawPaddle( const PongPaddle &paddle, uint16_t color )
{
    PongFillRectClipped( paddle.x, paddle.y, paddle.w, paddle.h, color );
}

/***************************************************
 * PongDrawBall()
 * 
 * Description: Draw a ball on the screen
 **************************************************/
static void PongDrawBall( const PongBall &ball, uint16_t color )
{
    PongFillRectClipped( ball.x, ball.y, ball.size, ball.size, color );
}

/***************************************************
 * PongResetBall()
 * 
 * Description: Reset the ball's position and
 *              velocity
 **************************************************/
static void PongResetBall( )
{
    g_ball.x = PONG_SCREEN_W / 2;
    g_ball.y = PONG_SCREEN_H / 2;
    g_ball.vx = 3;
    g_ball.vy = 2;
}

/***************************************************
 * PongResetGame()
 * 
 * Description: Reset the entire game state
 **************************************************/
static void PongResetGame( )
{
    g_player_paddle.x = PONG_PADDLE_OFFSET;
    g_player_paddle.y = PONG_SCREEN_H / 2 - PONG_PADDLE_H / 2;
    g_opponent_paddle.x = PONG_SCREEN_W - PONG_PADDLE_OFFSET - PONG_PADDLE_W;
    g_opponent_paddle.y = PONG_SCREEN_H / 2 - PONG_PADDLE_H / 2;
    PongResetBall();
}