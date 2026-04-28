/****************************************************
 * Errors.h
 * 
 * Common Error Codes
 * 
 ****************************************************/
#pragma once

typedef short mk_err_t;
enum
{
    ERR_NONE = 0,
    ERR_GNRL,
    ERR_FILE_NOT_FOUND,
    ERR_INVLD_PARAM,
    ERR_SD_MOUNT_FAIL,
};
