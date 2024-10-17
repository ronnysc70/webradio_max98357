#include "Audio.h" //see my repository at github "https://github.com/schreibfaul1/ESP32-audioI2S"
// Digital I/O used

#define I2S_DOUT      4
#define I2S_BCLK      5
#define I2S_LRC       6

Audio audio;

//start playing a stream from current active station
void startUrl()
{
  audio.connecttohost(stationlist[actStation].url);
}

// to be called in 'setup()'
void setup_audio()
{
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(4); // default 0...21
}
void audio_loop()
{
   audio.loop();
}

//**************************************************************************************************
//                                           E V E N T S                                           *
//**************************************************************************************************

void audio_info(const char *info)
{
    Serial.print("audio_info: "); 
    Serial.println(info);
}
