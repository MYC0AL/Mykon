/****************************************************
 * TaskSetup.h
 * 
 * Setup the system tasks
 * 
 ****************************************************/
#ifndef _TASKSETUP_H_
#define _TASKSETUP_H_

/**********************
 * Includes
 **********************/
#include <FreeRTOS.h>
#include <task.h>
#include "cmn/Errors.h"
#include "cmn/ControlTouch.h"

/**********************
 * Defines
 **********************/
#define TASK_MIN_STACK 4096

/**********************
 * Types
 **********************/
enum
{
    tskMED_PRIORITY,
    tskHIGH_PRIORITY,
};

typedef uint32_t ntfy_app_t8;
enum
{
    NTFY_NONE,
    NTFY_SETUP,
    NTFY_PRDC,
    NTFY_SUSPEND,
};

/* All User and Kernel Apps */
typedef uint8_t app_list_t8;
enum
{
    /* User Apps*/
    APP_TICTACTOE,
    APP_SLOTMACHINE,
    APP_TOUCHTIME,
    APP_USER_LAST = APP_TOUCHTIME,

    /* Kernel Apps */
    APP_MYKON,
    APP_DEV,
    APP_IO,
    APP_KRNL_LAST = APP_IO,

    APP_COUNT_TOTAL = APP_KRNL_LAST + 1,
    APP_COUNT_USER = APP_USER_LAST + 1,
    APP_COUNT_KRNL = APP_COUNT_TOTAL - APP_COUNT_USER,
    
};

/* Type alias */
using TaskSetupFunction = void (*)(void);

typedef struct 
{
    TaskHandle_t tsk_hndl;
    TaskSetupFunction setup_fnctn;

} Mykon_Hook_s;

/**********************
 * Function Prototypes
 **********************/
void GetMykonHooks( Mykon_Hook_s hooks[ APP_COUNT_TOTAL ] );
void MykonResume( );

/**********************
 * Variables
 **********************/

/**********************
 * Functions
 **********************/
mk_err_t Init_Task_Mykon( );
mk_err_t Init_Task_Dev( );
mk_err_t Init_Task_IO( );
mk_err_t Init_Task_TicTacToe( );
mk_err_t Init_Task_SlotMachine( );
mk_err_t Init_Task_TouchTime( );

#endif