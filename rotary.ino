
//handle events from rotary encoder
void rotary_loop()
{

   //process rotary encoder change
    if (rotaryEncoder.encoderChanged())
    {
      uint16_t v = rotaryEncoder.readEncoder();
      if (!(rotaryVol))
      {
        Serial.printf("Station: %i\n",v);
        uint8_t cnt = 0; //overflow counter, prevents endless loop if no station is enabled
        while ((!stationlist[v].enabled) && (cnt < 2))
        {
          v++;
          if (v >= STATIONS)
          {
            v=0;
            cnt++;
          }
        }
        //set new currtent station and show its name
        if (v < STATIONS) 
        {
          curStation = v;
          showStation();
          lastchange = millis();
        }
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("               ");         //Zeile zuerst löschen
        lcd.setCursor(0,1);
        sprintf(ausgabe,"Volume: %d", (v*2));  
        lcd.print(ausgabe);
        lcd.print("%");
        audio.setVolume(v); // default 0...21
        curVol = v;
        lastchange = millis();
      }
    }

  //if no change volume happened within 2s show Stationname
    if (rotaryVol && lastchange > 0 && (millis() - lastchange) > 2000)
    {
      pref.putUShort("volume",curVol);
      lastchange = 0;
      showStation();
    }
  
  //react on rotary encoder switch
    if (rotaryEncoder.isEncoderButtonClicked())
    {
      if (btnStation)
      {
        rotaryVol = true;
        btnStation = false;
       //set current station as active station and start streaming
        actStation = curStation;
        Serial.printf("Active station %s\n",stationlist[actStation].name);
        pref.putUShort("station",curStation);
        startUrl();
      //call show station to display the speaker symbol
        showStation();
        rotaryEncoder.setBoundaries(0, maxVol, false); //minValue, maxValue,
        rotaryEncoder.setEncoderValue(curVol);
      }
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
  rotaryEncoder.setBoundaries(0, maxVol, false); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
  rotaryEncoder.setEncoderValue(curVol);
  rotaryEncoder.disableAcceleration();  
}
