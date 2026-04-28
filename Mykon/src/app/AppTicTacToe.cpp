/****************************************************
 * AppTicTacToe.cpp
 * 
 * The TicTacToe Application
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "app/AppTicTacToe.h"

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
 * TicTacToe_setup()
 * 
 * Description: Setup the TicTacToe application
 **************************************************/
void TicTacToe_setup( )
{
    /* Clear screen */
    Display_getGFX()->fillScreen( BLACK );

}

/***************************************************
 * TicTacToe_run()
 * 
 * Description: Run the TicTacToe application
 **************************************************/
void TicTacToe_run( void * pvParameters )
{
    Serial.println("TicTacToe: Application Started ");

    /* Suspend self on startup */
    vTaskSuspend( NULL );

    uint8_t touch_count = 0;
    TP_Point touches[TOUCH_MAX] = {};

    TicTacToe_setup();

    TicTacToe ttt;
    ttt.DrawBoard();

    while( 1 )
    {
        Touch_getTouches( touches, &touch_count );
        if ( touch_count > 0 )
        {
            /* Process the first touch only */
            if ( ttt.PlacePiece( ttt.TileTouched( touches[0] ) ) == ERR_NONE )
            {
                ttt.ClearPieces();
                ttt.DrawBoard();

                /* Check if a player won */
                if ( ttt.GameOver() == ERR_NONE )
                {
                    vTaskDelay(1000);
                    ttt.ResetGame();
                    ttt.ClearPieces();
                    ttt.DrawBoard();
                    Touch_getTouches( touches, &touch_count );
                    memset( touches, 0, sizeof(TP_Point)*TOUCH_MAX );
                }
            }
        }
    }
}

/***************************************************
 * ResetGame()
 * 
 * Description: Reset the TTT game
 **************************************************/
mk_err_t TicTacToe::ResetGame()
{
    mk_err_t ret_val = ERR_NONE;

    if ((m_x_pos == 0) && (m_o_pos == 0))
    {
        ret_val = ERR_GNRL;
    }

    m_status = X_TURN;
    m_x_pos = 0;
    m_o_pos = 0;

    for(int i = 0; i < MAX_MOVE_COUNT; i++)
    {
        m_x_prev_moves[i] = 0;
        m_o_prev_moves[i] = 0;
    }

    m_x_move_count = 0;
    m_o_move_count = 0;

	return ret_val;
}

/***************************************************
 * DrawBoard()
 * 
 * Description: Draw the board
 **************************************************/
mk_err_t TicTacToe::DrawBoard()
{
    /* Get a handle to the graphics library */
    Arduino_ST7701_RGBPanel * gfx = Display_getGFX();

    /* Set text print size */
    gfx->setTextSize(15);

    for ( int bit = 0; bit < BOARD_SIZE; ++bit )
    {
        // Set cursor to next position
        gfx->setCursor( 165*(bit%3)+35,165*(bit/3)+20 );

        if ( m_x_pos & (1 << bit) )
        {
            gfx->setTextColor(CYAN);
            gfx->printf("X");
        }
        else if ( m_o_pos & (1 << bit) )
        {
            gfx->setTextColor(MAGENTA);
            gfx->printf("O");
        }
    }

    // Draw grid lines
    gfx->fillRect( 150,0,15,480,DARKGREY );
    gfx->fillRect( 315,0,15,480,DARKGREY );
    gfx->fillRect( 0,150,480,15,DARKGREY );
    gfx->fillRect( 0,315,480,15,DARKGREY );

    return ERR_NONE;
}

/***************************************************
 * ClearPieces()
 * 
 * Description: Clear all TTT pieces
 **************************************************/
mk_err_t TicTacToe::ClearPieces()
{
    for (int bit = 0; bit < BOARD_SIZE; ++bit)
    {
        /* Clear empty tiles */
        if ( getBit(bit) == 0 )
        {
            Display_getGFX()->fillRect( 165*(bit%3)+35,165*(bit/3)+20,90,120,BLACK );
        }
    }

    return ERR_NONE; 
}

/***************************************************
 * TileTouched()
 * 
 * Description: Check if a touch point is inside
 * any of the tiles
 **************************************************/
uint8_t TicTacToe::TileTouched( TP_Point tp )
{
    const int tile_w = Display_getGFX()->width() / 3;
    uint8_t x_coord = tp.x / tile_w;
    uint8_t y_coord = tp.y / tile_w;

    return ( y_coord * 3 + x_coord );
}

/***************************************************
 * PlacePiece()
 * 
 * Description: Place a piece on the board
 **************************************************/
mk_err_t TicTacToe::PlacePiece( uint8_t pos )
{

    if ( !VALID_POS(pos) )
    {
        return ERR_INVLD_PARAM;
    }

    if ( m_status == GAME_OVER )
    {
        return ERR_GNRL;
    }

    if ( getBit(pos) )
    {
        return ERR_GNRL;
    }

    if ( m_status == X_TURN )
    {
        setBit( m_x_pos, pos );

        removeOldestPiece(m_x_pos, m_x_prev_moves, m_x_move_count, pos);

        m_status = O_TURN;
    }
    else if ( m_status == O_TURN )
    {
        setBit( m_o_pos, pos );

        removeOldestPiece(m_o_pos, m_o_prev_moves, m_o_move_count, pos);

        m_status = X_TURN;
    }

    if ( GameOver() == ERR_NONE ) // Check if win
    {
        m_status = GAME_OVER;
    }

    return ERR_NONE;
}

/***************************************************
 * GameOver()
 * 
 * Description: Determine if the game is over
 **************************************************/
mk_err_t TicTacToe::GameOver()
{
    mk_err_t ret_val = ERR_GNRL;

    for (int i = 0; i < WIN_PATTERN_COUNT; i++)
    {
        if ((m_x_pos & WIN_PATTERNS[i]) == WIN_PATTERNS[i]) {
            ret_val = ERR_NONE;
        }
        else if ((m_o_pos & WIN_PATTERNS[i]) == WIN_PATTERNS[i]) {
            ret_val = ERR_NONE;
        }
    }

    return ret_val;
}

/***************************************************
 * setBit()
 * 
 * Description: Set a bit in the X or O field
 **************************************************/
mk_err_t TicTacToe::setBit( unsigned int& num, int pos, int val )
{
    mk_err_t ret_val = ERR_NONE;

    if (!VALID_POS(pos))
    {
        ret_val = ERR_INVLD_PARAM;
    }
    if (val != 0 && val != 1)
    {
        ret_val = ERR_INVLD_PARAM;
    }

    if (ret_val == ERR_NONE)
    {
        unsigned int mask = 1 << pos;
        num = ((num & ~mask) | (val << pos));
    }

    return ret_val;
}

/***************************************************
 * getBit()
 * 
 * Description: Get a bit in the X or O bit-field
 **************************************************/
int TicTacToe::getBit( uint8_t pos )
{
    int ret_val = -1;

    if (VALID_POS(pos))
    {
        unsigned int mask = 1 << pos;
        ret_val = ((m_x_pos | m_o_pos) & mask) >> pos;
    }

    return ret_val;
}

/***************************************************
 * removeOldestPiece()
 * 
 * Description: Remove oldest piece from X or O
 **************************************************/
mk_err_t TicTacToe::removeOldestPiece( unsigned int& num, unsigned int prev_moves[MAX_MOVE_COUNT], int& count, int pos )
{
    int offset = count % MAX_MOVE_COUNT;

    int oldest_move = prev_moves[MAX_MOVE_COUNT-1];

    if (count >= 3)
    {
        // Mask out the oldest previous move
        num ^= prev_moves[offset];
    }

    prev_moves[offset] &= 0b000000000;
    prev_moves[offset] = (1 << pos);

    count++;

    return ERR_NONE;
}