/****************************************************
 * DrawJPEG.h
 * 
 * Functions to read a file from the SD card
 * and display them to the screen.
 * 
 ****************************************************/
#pragma once

/**********************
 * Includes
**********************/
#include <JPEGDEC.h>
#include <SD.h>
#include "cmn/Errors.h"

/**********************
 * Variables
 **********************/

/**********************
 * Functions
 **********************/
mk_err_t DrawJPEG( const char *filename, JPEG_DRAW_CALLBACK *jpegDrawCallback, 
               bool useBigEndian, int x, int y, int widthLimit, int heightLimit );

