//init WiFi Verbindung
int setup_wifi()
{       
  Serial.println("Connecting to WiFi");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  if (tryCount < MAXWLANTRY) {
    // Mit Wi-Fi verbinden
    Serial.println("...Connecting to WiFi");
    lcdPrint(0, 0, "suche WLAN...");
    WiFi.mode(WIFI_STA);
    if ((millis() - delayTimeRefresh) > 2000)       //aller 2 Sekunden Überprüfung WLAN und Internet Verbindung
    {
      int retry = 0;
      delayTimeRefresh = millis();
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("reconnect WIFI");
        lcd.clear();
        lcdPrint(0, 0, "Huch, WLAN weg..");
        lcdPrint(0, 1, "...verbinde neu");
        WiFi.disconnect();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.begin(SSID, PSK);
        while (WiFi.status() != WL_CONNECTED)
        {
          Serial.print(".");
          delay(100);
        }
        if (btnStandby)
        {
          showStandby();
        }
        else
        {
          showStation();
        }
      }
      // teste ob eine Verbindung zum Internet besteht wg. capitive Portal
 
      else
      {
        while((!httpsClient.connect("www.google.de", 443, 2000)) && (retry < 5))  //3. Wert ist timeout der Verbindung in ms
        {
          delay(100);
          Serial.print("<*>");
          retry++;   
        }
        if (retry==5)
        {
          Serial.println("keine Verbindung");
          lcd.clear();
          lcdPrint(0, 0, "kein Internet..");
          lcdPrint(0, 1, "..neu anmelden");
          internetLost = true;
        }
        else
        {
          Serial.println("Internet erreichbar");
          if (internetLost)
          {
            internetLost = false;
            Serial.println("Internet wieder erreichbar, starte Stream neu");
            if (btnStandby)
            {
              showStandby();
            }
            else
            {
              showStation();
              startUrl();
            }
          }
        }
        httpsClient.stop();
      }
    }
    WiFi.begin(SSID, PSK);
     while ((WiFi.status() != WL_CONNECTED) && (tryCount<MAXWLANTRY)) 
     {
       Serial.print(".");
       lcdPrint(tryCount, 1, ">");
       delay(1000);
       tryCount++;
    }
    IPAddress lip(0,0,0,0);
    if (WiFi.localIP() == lip) {
      tryCount = MAXWLANTRY;
    }
  }
  if (tryCount < MAXWLANTRY) {
    Serial.print(F("IP-Adresse per DHCP ist "));
    Serial.println(WiFi.localIP());
    return(true);   // when connection ok, stop here and return positive
  }
  return(false);
}

void wifi_loop()
{
  
    if ((millis() - delayTimeRefresh) > 2000)       //aller 2 Sekunden Überprüfung WLAN und Internet Verbindung
    {
      int retry = 0;
      delayTimeRefresh = millis();
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("reconnect WIFI");
        lcd.clear();
        lcdPrint(0, 0, "Huch, WLAN weg..");
        lcdPrint(0, 1, "...verbinde neu");
        WiFi.disconnect();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.begin(SSID, PSK);
        while (WiFi.status() != WL_CONNECTED)
        {
          Serial.print(".");
          delay(100);
        }
        if (btnStandby)
        {
          showStandby();
        }
        else
        {
          showStation();
        }
      }
      // teste ob eine Verbindung zum Internet besteht wg. capitive Portal
 
      else
      {
        while((!httpsClient.connect("www.google.de", 443, 2000)) && (retry < 3))  //3. Wert ist timeout der Verbindung in ms
        {
          delay(100);
          Serial.print("<*>");
          retry++;   
        }
        if (retry==3)
        {
          Serial.println("keine Verbindung");
          lcd.clear();
          lcdPrint(0, 0, "kein Internet..");
          lcdPrint(0, 1, "..neu anmelden");
          internetLost = true;
        }
        else
        {
          Serial.println("Internet erreichbar");
          if (internetLost)
          {
            internetLost = false;
            Serial.println("Internet wieder erreichbar, starte Stream neu");
            if (btnStandby)
            {
              showStandby();
            }
            else
            {
              showStation();
              startUrl();
            }
          }
        }
        httpsClient.stop();
      }
    }
}
