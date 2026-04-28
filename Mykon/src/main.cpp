
#include "cmn/DrawJPEG.h"
#include "cmn/ControlSD.h"
#include "cmn/ControlDisplay.h"
#include "cmn/ControlTouch.h"
#include "knl/TaskSetup.h"
#include "cmn/Config.h"

void setup()
{
  Serial.begin(9600);

  Touch_Init();
  Serial.println("Mykon: Touch Driver Initialized");

  Display_getCanvas()->begin();
  Serial.println("Mykon: GFX Canvas Initialized");

  if ( SD_mount() == ERR_NONE )
  {
    Serial.println("Mykon: SD Card Mounted");
  }
  else
  {
    Serial.println("Error: SD Card Failed to Mount");
  }

  /* Mykon MUST be initialized because it controls resource management */
  Init_Task_Mykon();

  #if ( CFG_DEV )
  Init_Task_Dev();
  #endif

  /* Initialize system tasks */
  Init_Task_IO();

  /* Initialize the relevant tasks */
  Init_Task_TicTacToe();
  Init_Task_SlotMachine();
  Init_Task_TouchTime();


}

void loop(void)
{
}