/****************************************************
 * TaskSetup.cpp
 * 
 * Setup the Mykon tasks
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "knl/TaskSetup.h"
#include "app/AppMykon.h"
#include "app/AppDev.h"
#include "app/AppIO.h"
#include "app/AppTicTacToe.h"
#include "app/AppSlotMachine.h"
#include "app/AppTouchTime.h"
#include "cmn/Config.h"

/**********************
 * Defines
 **********************/

/**********************
 * Function Prototypes
 **********************/

/**********************
 * Variables
 **********************/
static Mykon_Hook_s _hooks[ APP_COUNT_TOTAL ];

/**********************
 * Functions
 **********************/
/***************************************************
 * GetMykonHooks()
 * 
 * Description: Return a struct of all task handles
 **************************************************/
void GetMykonHooks( Mykon_Hook_s hooks[ APP_COUNT_TOTAL ] )
{
    memcpy( hooks, _hooks, sizeof( Mykon_Hook_s ) * APP_COUNT_TOTAL );
}

/***************************************************
 * Init_Task_Mykon()
 * 
 * Description: Create the Mykon task and app
 **************************************************/
mk_err_t Init_Task_Mykon( )
{
    xTaskCreate
        (
        Mykon_run,
        "Mykon",
        TASK_MIN_STACK,
        nullptr,
        tskMED_PRIORITY,
        &_hooks[ APP_MYKON ].tsk_hndl
        );

    _hooks[ APP_MYKON ].setup_fnctn = nullptr;

    return ERR_NONE;
}

/***************************************************
 * Init_Task_Dev()
 * 
 * Description: Creat the Dev task
 **************************************************/
mk_err_t Init_Task_Dev( )
{
    xTaskCreate
        (
        Dev_run,
        "Dev",
        TASK_MIN_STACK,
        nullptr,
        tskMED_PRIORITY,
        &_hooks[ APP_DEV ].tsk_hndl
        );

    _hooks[ APP_DEV ].setup_fnctn = nullptr;
    return ERR_NONE;
}

/***************************************************
 * Init_Task_IO()
 * 
 * Description: Create the IO task and app
 **************************************************/
mk_err_t Init_Task_IO( )
{
    xTaskCreate
        (
        IO_run,
        "IO",
        TASK_MIN_STACK,
        nullptr,
        tskMED_PRIORITY,
        &_hooks[ APP_IO ].tsk_hndl
        );

    _hooks[ APP_IO ].setup_fnctn = IO_setup;

    return ERR_NONE;
}

/***************************************************
 * Init_Task_TicTacToe()
 * 
 * Description: Create the TicTacToe task
 **************************************************/
mk_err_t Init_Task_TicTacToe( )
{
    xTaskCreate
        (
        TicTacToe_run,
        "TicTacToe",
        TASK_MIN_STACK,
        nullptr,
        tskMED_PRIORITY,
        &_hooks[ APP_TICTACTOE ].tsk_hndl
        );

    _hooks[ APP_TICTACTOE ].setup_fnctn = TicTacToe_setup;
    return ERR_NONE;
}

/***************************************************
 * Init_Task_SlotMachine()
 * 
 * Description: Create the Slot Machine task
 **************************************************/
mk_err_t Init_Task_SlotMachine( )
{
    xTaskCreate
        (
        SlotMachine_run,
        "SlotMachine",
        TASK_MIN_STACK,
        nullptr,
        tskMED_PRIORITY,
        &_hooks[ APP_SLOTMACHINE ].tsk_hndl
        );

    _hooks[ APP_SLOTMACHINE ].setup_fnctn = SlotMachine_setup;

    return ERR_NONE;  
}

/***************************************************
 * Init_Task_TouchTime()
 * 
 * Description: Create the Touch Time task
 **************************************************/
mk_err_t Init_Task_TouchTime( )
{
    xTaskCreate
        (
        TouchTime_run,
        "TouchTime",
        TASK_MIN_STACK,
        nullptr,
        tskMED_PRIORITY,
        &_hooks[ APP_TOUCHTIME ].tsk_hndl
        );

    _hooks[ APP_TOUCHTIME ].setup_fnctn = TouchTime_setup;
    return ERR_NONE;
}