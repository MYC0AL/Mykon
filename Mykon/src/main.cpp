
#include "cmn/DrawJPEG.h"
#include "cmn/ControlSD.h"
#include "cmn/ControlDisplay.h"
#include "cmn/ControlTouch.h"
#include "knl/TaskSetup.h"
#include "cmn/Config.h"

static void scan_i2c( )
{
  Serial.println("Scanning...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }

  Serial.println("Done");
  while(1);
}

void setup()
{
  /* Initialize Serial Communication */
  Serial.begin(9600);

  /* Initialize I2C communication */
  Wire.begin( I2C_SDA_PIN, I2C_SCL_PIN );
  Wire.setClock( 100000 );

  /* Initialize the display canvas */
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

  // /* Mykon MUST be initialized because it controls resource management */
  Init_Task_Mykon();

  #if ( CFG_DEV )
  Init_Task_Dev();
  #endif

  // /* Initialize system tasks */
  Init_Task_IO();
  // Init_Task_Audio();

  // /* Initialize the relevant tasks */
  Init_Task_TicTacToe();
  Init_Task_SlotMachine();
  Init_Task_TouchTime();

}

void loop(void)
{
}