//Meldungen einzelne Zeile
int lcdPrint(int x, int y, const char *msg) 
{
  lcd.setCursor(x, y);
  lcd.print(msg);
  return 0;
}
//show Standby Anzeige
void showStandby()
{
  time(&now); // read the current time
  localtime_r(&now, &tm);             // update the structure tm with the current tim
  lcd.setCursor(6,0);
  sprintf(ausgabe, "%02d:%02d", tm.tm_hour, tm.tm_min);
  lcd.print(ausgabe);
  lcd.setCursor(3,1);
  sprintf(ausgabe, "%02d.%02d.%02d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
  lcd.print(ausgabe);  
  
}
//show name of current station on LCD display
//show the speaker symbol in front if current station = active station
void showStation() 
{
  uint8_t loff=0;
  lcd.clear();
  if (curStation == actStation) 
  {
    lcd.home();
    if (!(streamReady))
    {
      lcd.print(char(1));   // offener Lautstprecher Symbol
    }
    else
    {
      lcd.print(char(2));   // gefülltes Lautsprecher Symbol
    }
    
    loff=2;
  }
  if (btnStation)   //wenn Taster gedrückt kein Symbol am Anfang
  {
    loff=0;
  }
  lcd.setCursor(loff, 0);
  lcd.print(curStation+1);
  if (curStation > 8) {
    lcd.setCursor((loff+2), 0);
  } 
  else {
    lcd.setCursor((loff+1), 0);
  }
  lcd.print(":");
  String name = String(stationlist[curStation].name);
  if (name.length() < 12-loff)
    lcd.print(name);
  else {
    uint8_t p = name.lastIndexOf(" ",(15-loff)); //if name does not fit, split line on space
    lcd.print(name.substring(0,p));
    lcd.setCursor(0,1);
    lcd.print(name.substring(p+1,p+17));
  }
}
