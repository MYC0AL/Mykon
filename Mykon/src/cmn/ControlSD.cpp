/****************************************************
 * ControlSD.cpp
 * 
 * Helper functions for interfacing with SD card.
 * 
 ****************************************************/

/**********************
 * Includes
**********************/
#include "cmn/ControlSD.h"

/**********************
 * Variables
 **********************/

/**********************
 * Functions
 **********************/

/***************************************************
 * SD_mount()
 * 
 * Description: Mount the SD Card to access the
 * files.
 **************************************************/
mk_err_t SD_mount( )
{
    mk_err_t retCode = ERR_NONE;

    SPI.begin( SD_SCK, SD_MISO, SD_MOSI, SD_CS );
    pinMode(SD_CS, OUTPUT);

    SPI.setDataMode( SPI_MODE0 );
    if (!SD.begin( SD_CS ))
    {
        retCode = ERR_SD_MOUNT_FAIL;
    }

    return retCode;
}

/***************************************************
 * SD_getFile()
 * 
 * Description: Based on a file name, open the SD
 * card and return a pointer to the file.
 **************************************************/
mk_err_t SD_getFile( File* sd_file, const char *filename, int32_t *size ) 
{
    if ( !sd_file )
    {
        return ERR_INVLD_PARAM;
    }

    *sd_file = SD.open( filename ); 
    *size = sd_file->size();

    return ERR_NONE;
}

/***************************************************
 * SD_closeFile()
 * 
 * Description: Close an open file on the SD card.
 * 
 **************************************************/
mk_err_t SD_closeFile( File* sd_file )
{
    if ( !sd_file )
    {
        return ERR_INVLD_PARAM;
    }

    if ( sd_file )
    {
        sd_file->close();
    }

    return ERR_NONE;
}

/***************************************************
 * SD_readFile()
 * 
 * Description: Read SD card file into buffer.
 * 
 **************************************************/
mk_err_t SD_readFile( File* sd_file, uint8_t *buffer, int32_t length )
{
    if ( !sd_file )
    {
        return ERR_INVLD_PARAM;
    }

    sd_file->read( buffer, length );

    return ERR_NONE;
}
