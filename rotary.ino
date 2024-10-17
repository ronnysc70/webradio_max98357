//library for rotary encoder
#include "AiEsp32RotaryEncoder.h"
//used pins for rotary encoder heltec V3
#ifdef heltec
  #define ROTARY_ENCODER_A_PIN 47
  #define ROTARY_ENCODER_B_PIN 48
  #define ROTARY_ENCODER_BUTTON_PIN 26
  #define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
#endif

#ifdef ESP32S3Dev
  #define ROTARY_ENCODER_A_PIN 15
  #define ROTARY_ENCODER_B_PIN 16
  #define ROTARY_ENCODER_BUTTON_PIN 7
  #define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
#endif

//depending on your encoder - try 1,2 or 4 to get expected behaviour
//#define ROTARY_ENCODER_STEPS 1
//#define ROTARY_ENCODER_STEPS 2
#define ROTARY_ENCODER_STEPS 4

//instance for rotary encoder
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);
/*
int setEncoderValue(int value)
{
  if (value >= STATIONS) value = STATIONS - 1;
  if (value < 0) value = 0;
  rotaryEncoder.setEncoderValue(value);
  return 0;
}
*/
//handle events from rotary encoder
void rotary_loop()
{
  //dont do anything unless value changed
  if (rotaryEncoder.encoderChanged())
  {
    uint16_t v = rotaryEncoder.readEncoder();
    Serial.printf("Station: %i\n",v);
    //set new currtent station and show its name
    if (v < STATIONS) {
      curStation = v;
      showStation();
      lastchange = millis();
    }
  }
  //if no change happened within 10s set active station as current station
  if (curStation != actStation && lastchange > 0 && (millis() - lastchange) > 10000){
    curStation = actStation;
    lastchange = 0;
    showStation();
  }
  //react on rotary encoder switch
  if (rotaryEncoder.isEncoderButtonClicked())
  {
    //set current station as active station and start streaming
    actStation = curStation;
    Serial.printf("Active station %s\n",stationlist[actStation].name);
    pref.putUShort("station",curStation);
    startUrl();
    //call show station to display the speaker symbol
    showStation();
  }
}

//interrupt handling for rotary encoder
void IRAM_ATTR readEncoderISR()
{
  rotaryEncoder.readEncoder_ISR();
}

void setup_rotary()
{
  //start rotary encoder instance
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  rotaryEncoder.setBoundaries(0, STATIONS, true); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
  rotaryEncoder.disableAcceleration();  
}
