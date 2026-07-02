/****************************************************
 * sdl.h
 * 
 * The SDL/graphics driver
 * 
 ****************************************************/
#ifndef SDL_H
#define SDL_H

/**********************
 * Includes
 **********************/
#include <Arduino.h>

/**********************
 * Defines
 **********************/
#define DRAW_HEIGHT             ( 144 )
#define DRAW_WIDTH              ( 160 )
#define SCREEN_HEIGHT           ( 480 )
#define SCREEN_WIDTH            ( 480 )
#define SCALE_X                 ( 3 )
#define SCALE_Y                 ( 3 )
#define SCALED_DRAW_WIDTH       (DRAW_WIDTH * SCALE_X)
#define SCALED_DRAW_HEIGHT      (DRAW_HEIGHT * SCALE_Y)
#define SCREEN_OFFSET_Y         ((SCREEN_HEIGHT - SCALED_DRAW_HEIGHT) / 2)

/**********************
 * Types
 **********************/
typedef struct
{
    uint8_t up : 1;
    uint8_t down : 1;
    uint8_t left : 1;
    uint8_t right : 1;

    uint8_t start : 1;
    uint8_t select : 1;

    uint8_t a : 1;
    uint8_t b : 1;
    
} Gameboy_Buttons_s;

 /**********************
 * Function Prototypes
 **********************/
void sdl_init( void );
uint8_t *sdl_get_framebuffer( void );
unsigned int sdl_get_buttons( void );
unsigned int sdl_get_directions( void );
void sdl_read_buttons( void );
TaskHandle_t sdl_get_draw_task_handle( void );
void sdl_set_buttons( const Gameboy_Buttons_s * buttons );

#endif
