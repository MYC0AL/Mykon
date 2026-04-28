/****************************************************
 * ControlSD.h
 * 
 * Helper functions for interfacing with SD card.
 * 
 ****************************************************/
#pragma once

/**********************
 * Includes
 **********************/
#include <SD.h>
#include "cmn/Errors.h"

/**********************
 * Defines
 **********************/
#define SD_SCK  12
#define SD_MISO 13
#define SD_MOSI 11
#define SD_CS   10

mk_err_t SD_mount( );
mk_err_t SD_getFile( File* sd_file, const char *filename, int32_t *size );
mk_err_t SD_closeFile( File* sd_file );
mk_err_t SD_readFile( File* sd_file, uint8_t *buffer, int32_t length );