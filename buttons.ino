// process button press:
void button_loop()
{
  //Standby-Taster auswerten
  int buttonPressedStandby = digitalRead(standbyButton);
  if (buttonPressedStandby == 0)
  {
    buttonTimeStamp = millis();                        //Entprellung, bounce2 geht nicht
    if (buttonTimeStamp - buttonPressedTime > 200)   //200msek. mind. Signal
    {
      if (!(buttonPressed))             //nur einmal ausführen
      {
        if (!(btnStandby))
        {
          audio.stopSong();
          lcd.clear();
          lcd.home();
          lcd.print("Auf Wiedersehen");
          delay(3000);
          lcd.clear();
          showStandby();
          btnStandby = true;
          Serial.println("standby");
        }
        else
        {
          rotaryVol = true;
          startUrl();
          //call show station to display the speaker symbol
          showStation();
          rotaryEncoder.setBoundaries(0, maxVol, false); //minValue, maxValue,
          rotaryEncoder.setEncoderValue(curVol);
          btnStandby = false;
        }
        pref.putBool("standby", btnStandby);
        buttonPressed = true;
        buttonPressedTime = buttonTimeStamp;
      }
    }
  }
  else
  {
    buttonPressed = false;
  }
  //die anderen Taster auswerten, aber nur wenn kein Standby
  if (!(btnStandby))
  {
    int buttonPressedFav = digitalRead(favButton);
    if (buttonPressedFav == 0)
    {
      buttonTimeStamp = millis();                        //Entprellung, bounce2 geht nicht
      if (buttonTimeStamp - buttonPressedTime > 100)     //100msek. mind. Signal
      {
        rotaryVol = false;
        rotaryEncoder.setBoundaries(0, STATIONS, true); //minValue, maxValue,
        rotaryEncoder.setEncoderValue(actStation);
        btnStation = true;
        showStation();
        lastchange = millis();
        buttonPressedTime = buttonTimeStamp;
      }
    }
    int buttonPressedMode = digitalRead(modeButton);
    if (buttonPressedMode == 0)
    {
      buttonTimeStamp = millis();                        //Entprellung, bounce2 geht nicht
      if (buttonTimeStamp - buttonPressedTime > 100)     //100msek. mind. Signal
      {
        if (!(buttonPressed))           //nur einmal ausführen
        {
          btnMode = true;
          lcd.clear();
          lcd.home();
          lcd.print("IP-Adresse:");
          lcdPrint(0, 1, WiFi.localIP().toString().c_str());
          lastchange = millis();
          buttonPressed = true;
        }
        buttonPressedTime = buttonTimeStamp;
      }
    }
    else
    {
      buttonPressed = false;
    }
   //if no change station happened within 5s set active station as current station
    if (btnStation && (millis() - lastchange) > 5000)
    {
      curStation = actStation;
      lastchange = 0;
      rotaryVol = true;
      btnStation = false;
      showStation();
    }
  //if no change station happened within 3s set active station as current station
    if (btnMode && (millis() - lastchange) > 3000)
    {
      curStation = actStation;
      lastchange = 0;
      rotaryVol = true;
      btnMode = false;
      showStation();
    }
  }
}
