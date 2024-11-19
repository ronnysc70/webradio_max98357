// process button press:
void button_loop()
{
  // process button press:
  for (int i = 0; i<NUMBUTTONS; i++) 
  {
     buttons[i].update(); // Update the Bounce instance
     if ( buttons[i].fell() ) // If it fell
     {
       switch(i) {
        case 0:                     //Standby-Taster auswerten
                if (!(btnStandby))
                {
                  audio.stopSong();
                  lcd.clear();                      //ESP32S3
                  lcd.home();
                  lcd.print("Auf Wiedersehen");
                  delay(2000);
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
                break;
        case 1:                             //Mode Taster
                if (!(btnStandby))
                {
                  btnMode = true;
                  lcd.clear();                      //ESP32S3
                  lcd.home();
                  lcd.print("IP-Adresse:");
                  lcdPrint(0, 1, WiFi.localIP().toString().c_str());
                  lastchange = millis();
                }
                break;
        case 2:                           //Fav-Taster
                if (!(btnStandby))
                {
                  btnStation = true;
                  rotaryVol = false;
                  rotaryEncoder.setBoundaries(0, STATIONS, true); //minValue, maxValue,
                  rotaryEncoder.setEncoderValue(actStation);
                  showStation();
                  lastchange = millis();
                }
                break;
       }
     }
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
  //nach 3 sek. Anzeige IP-Adresse zurück zur Stationsanzeige 
    if (btnMode && (millis() - lastchange) > 3000)
    {
      curStation = actStation;
      lastchange = 0;
      rotaryVol = true;
      btnMode = false;
      showStation();
    }
}
