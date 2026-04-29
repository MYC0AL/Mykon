/****************************************************
 * AppAudio.cpp
 * 
 * The Audio Application
 * 
 ****************************************************/

/**********************
 * Includes
 **********************/
#include "app/AppAudio.h"

/**********************
 * Defines
 **********************/

/**********************
 * Function Prototypes
 **********************/

/**********************
 * Variables
 **********************/
AudioGeneratorMP3    *audio_gen_mp3;
AudioFileSourceSD    *audio_file;
AudioOutputI2S       *audio_out;

/**********************
 * Functions
 **********************/
 /***************************************************
 * Audio_setup()
 * 
 * Description: Setup the Audio application
 **************************************************/
void Audio_setup( )
{
audio_out = new AudioOutputI2S();
audio_out->SetPinout( AUDIO_PIN_BCLK, AUDIO_PIN_LRCK, AUDIO_PIN_DOUT );
audio_out->SetGain(0.05);

audio_file = new AudioFileSourceSD( AUDIO_FILENAME_01 );

if ( !audio_file->isOpen() )
{
    Serial.println("Audio: Failed to open audio file");
    return;
}

audio_gen_mp3 = new AudioGeneratorMP3();
//audio_gen_mp3->begin(audio_file, audio_out);
}

/***************************************************
 * Audio_run()
 * 
 * Description: Run the Audio application
 **************************************************/
void Audio_run( void * pvParameters )
{
    Serial.println("Audio: Application Started");

    /* Setup task */
    Audio_setup();

    while(1)
    {
        if (audio_gen_mp3->isRunning())
        {
            audio_gen_mp3->loop();
        }

        vTaskDelay(1);

    }
}
