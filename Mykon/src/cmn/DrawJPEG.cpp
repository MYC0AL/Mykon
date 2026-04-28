/****************************************************
 * DrawJPEG.h
 * 
 * Functions to read a JPEG from the SD card
 * and display them to the screen.
 * 
 ****************************************************/

/**********************
 * Includes
**********************/
#include "cmn/DrawJPEG.h"

/**********************
 * Function Prototypes
**********************/
static void *jpegOpenFile( const char *szFilename, int32_t *pFileSize );
static void jpegCloseFile( void *pHandle );
static int32_t jpegReadFile( JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen );
static int32_t jpegSeekFile(JPEGFILE *pFile, int32_t iPosition);

/**********************
 * Variables
 **********************/
static JPEGDEC _jpeg;
static File _f;
static int _x, _y, _x_bound, _y_bound;

/**********************
 * Functions
 **********************/

/***************************************************
 * DrawJPEG()
 * 
 * Description: Use the callback function to draw
 * JPEG pixels to the display
 **************************************************/
mk_err_t DrawJPEG( const char *filename, JPEG_DRAW_CALLBACK *jpegDrawCallback, 
               bool useBigEndian, int x, int y, int widthLimit, int heightLimit )
{
    _x = x;
    _y = y;
    _x_bound = _x + widthLimit - 1;
    _y_bound = _y + heightLimit - 1;

    _jpeg.open(filename, jpegOpenFile, jpegCloseFile, jpegReadFile, jpegSeekFile, jpegDrawCallback);

    // scale to fit height
    int _scale;
    int iMaxMCUs;
    float ratio = (float)_jpeg.getHeight() / heightLimit;
    if (ratio <= 1)
    {
        _scale = 0;
        iMaxMCUs = widthLimit / 16;
    }
    else if (ratio <= 2)
    {
        _scale = JPEG_SCALE_HALF;
        iMaxMCUs = widthLimit / 8;
    }
    else if (ratio <= 4)
    {
        _scale = JPEG_SCALE_QUARTER;
        iMaxMCUs = widthLimit / 4;
    }
    else
    {
        _scale = JPEG_SCALE_EIGHTH;
        iMaxMCUs = widthLimit / 2;
    }
    _jpeg.setMaxOutputSize(iMaxMCUs);
    if (useBigEndian)
    {
        _jpeg.setPixelType(RGB565_BIG_ENDIAN);
    }
    _jpeg.decode(x, y, _scale);
    _jpeg.close();
    
    return ERR_NONE;
}

/***************************************************
 * jpegOpenFile()
 * 
 * Description: Open a file on the SD card
 **************************************************/
static void *jpegOpenFile( const char *szFilename, int32_t *pFileSize )
{
    _f = SD.open(szFilename, FILE_READ );
    *pFileSize = _f.size();
    return &_f;
}

/***************************************************
 * jpegCloseFile()
 * 
 * Description: Close a file on the SD card
 **************************************************/
static void jpegCloseFile( void *pHandle )
{
    File *f = static_cast<File *>(pHandle);
    f->close();
}

/***************************************************
 * jpegReadFile()
 * 
 * Description: Read a file on the SD card
 **************************************************/
static int32_t jpegReadFile( JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen )
{
    File *f = static_cast<File *>(pFile->fHandle);
    size_t read_count = f->read(pBuf, iLen);
    return read_count;
}

/***************************************************
 * jpegSeekFile()
 * 
 * Description: Seek a file on the SD card
 **************************************************/
static int32_t jpegSeekFile(JPEGFILE *pFile, int32_t iPosition)
{
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    return iPosition;
}

