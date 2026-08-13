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
#include "app/AppAudio.h"
#include "app/AppTicTacToe.h"
#include "app/AppSlotMachine.h"
#include "app/AppTouchTime.h"
#include "app/AppPong.h"
#include "app/AppGameBoy.h"
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
/* All App Hooks */
static Mykon_Hook_s _hooks[ APP_COUNT_TOTAL ];

/* Apps that want IO notifcations */
static app_list_t8 _app_io_ntfy[ ] =
{
    APP_MYKON,
    APP_DEV,
    APP_TICTACTOE,
    APP_SLOTMACHINE,
    APP_TOUCHTIME,
};

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

    _hooks[ APP_MYKON ].setup_fnctn = Mykon_setup;
    _hooks[ APP_MYKON ].app_sbscrptn = APP_IO;

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
    _hooks[ APP_DEV ].app_sbscrptn = APP_IO;

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
    _hooks[ APP_IO ].app_sbscrptn = APP_COUNT_TOTAL;    /* No subscriptions */

    return ERR_NONE;
}

/***************************************************
 * Init_Task_Audio()
 * 
 * Description: Create the Audio task and app
 **************************************************/
mk_err_t Init_Task_Audio( )
{
    xTaskCreate
        (
        Audio_run,
        "Audio",
        TASK_MIN_STACK,
        nullptr,
        tskMED_PRIORITY,
        &_hooks[ APP_AUDIO ].tsk_hndl
        );

    _hooks[ APP_AUDIO ].setup_fnctn = Audio_setup;
    _hooks[ APP_AUDIO ].app_sbscrptn = APP_COUNT_TOTAL;    /* No subscriptions */

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
    _hooks[ APP_TICTACTOE ].app_sbscrptn = APP_IO;

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
    _hooks[ APP_SLOTMACHINE ].app_sbscrptn = APP_IO;

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
    _hooks[ APP_TOUCHTIME ].app_sbscrptn = APP_IO;

    return ERR_NONE;
}

/***************************************************
 * Init_Task_Pong()
 * 
 * Description: Create the Pong task
 **************************************************/
mk_err_t Init_Task_Pong( )
{
    xTaskCreate
        (
        Pong_run,
        "Pong",
        TASK_MIN_STACK,
        nullptr,
        tskMED_PRIORITY,
        &_hooks[ APP_PONG ].tsk_hndl
        );

    _hooks[ APP_PONG ].setup_fnctn = Pong_setup;
    _hooks[ APP_PONG ].app_sbscrptn = APP_IO;

    return ERR_NONE;
}

/***************************************************
 * Init_Task_GameBoy()
 * 
 * Description: Create the Game Boy task
 **************************************************/
mk_err_t Init_Task_GameBoy( )
{
    xTaskCreatePinnedToCore
        (
        GameBoy_run,
        "GameBoy",
        TASK_MIN_STACK,
        nullptr,
        tskMED_PRIORITY,
        &_hooks[ APP_GAMEBOY ].tsk_hndl,
        1 );

    _hooks[ APP_GAMEBOY ].setup_fnctn = GameBoy_setup;
    _hooks[ APP_GAMEBOY ].app_sbscrptn = APP_IO;

    return ERR_NONE;
}