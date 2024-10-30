#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h> 
#include <Arduino.h>
#include <Wire.h>
#include <time.h>

/* Configuration of NTP */
// choose the best fitting NTP server pool for your country
#define NTP_SERVER "de.pool.ntp.org"

// choose your time zone from this list
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"

//Webserver instance
WebServer server(80);

//esp32 library to save preferences in flash
#include <Preferences.h>

//library for LCD display
#include <LiquidCrystal_I2C.h>

//instance for LCD display
//Pins ESP32S3: SDA: 42, SCL: 41
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
//I2C def
#define SDA 42
#define SCL 41

//Audio instance
#include "Audio.h" //see my repository at github "https://github.com/schreibfaul1/ESP32-audioI2S"
// Digital I/O used

#define I2S_DOUT      4
#define I2S_BCLK      5
#define I2S_LRC       6

Audio audio;

//WiFI Einstellungen in der credentials.h
#include <credentials.h>
#define MAXWLANTRY 10  // try to connect with stored credentials MAXWLANTRY times
int tryCount = 0;

//Buttons
#define favButton 21
#define modeButton 47
#define standbyButton 38
unsigned long buttonTimeStamp = 0;
unsigned long buttonPressedTime = 0;
bool buttonPressed = false;

//library for rotary encoder
#include "AiEsp32RotaryEncoder.h"
#define ROTARY_ENCODER_A_PIN 15
#define ROTARY_ENCODER_B_PIN 16
#define ROTARY_ENCODER_BUTTON_PIN 7
#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */

//depending on your encoder - try 1,2 or 4 to get expected behaviour
//#define ROTARY_ENCODER_STEPS 1
//#define ROTARY_ENCODER_STEPS 2
#define ROTARY_ENCODER_STEPS 4

//instance for rotary encoder
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);


//structure for station list
typedef struct {
  char * url;  //stream url
  char * name; //stations name
} Station;


#define STATIONS 10 //number of stations in the list

//station list can easily be modified to support other stations  
Station stationlist[STATIONS] PROGMEM = {
{"https://mdr-284290-2.sslcast.mdr.de/mdr/284290/2/mp3/high/stream.mp3","MDR Sachsen Anhalt"},
{"http://stream.radiobrocken.de/live/mp3-256/play.m3u","Radio Brocken"},
{"https://stream.radiosaw.de/saw/mp3-192","SAW"},
{"http://stream.89.0rtl.de/live/mp3-256/", "89,0 RTL"},
{"https://mdr-284330-0.sslcast.mdr.de/mdr/284330/0/mp3/high/stream.mp3","MDR Sputnik"},
{"https://absolut-relax.live-sm.absolutradio.de/absolut-relax/stream/mp3", "Absolut Relax"},
{"https://stream.saw-musikwelt.de/saw-in-the-mix/mp3-192", "Radio SAW in the Mix"},
{"http://mdr-284331-2.sslcast.mdr.de/mdr/284331/2/mp3/high/stream.mp3","MDR Sputnik in the mix"},
{"http://stream.89.0rtl.de/mix/mp3-256/play.m3u", "89,0 RTL in the Mix"}, 
{"http://stream.sunshine-live.de/live/mp3-192","Sunshine Live"}
};


//instance of prefernces
Preferences pref;

//Special character to show a speaker icon for current station
uint8_t speaker[8]  = {0x03,0x05,0x19,0x11,0x19,0x05,0x03};
uint8_t fullspeaker[8] = {0x03,0x07,0x1F,0x1F,0x1F,0x1F,0x07,0x03};

//global variables
char ausgabe[20];                   // für sprintf
unsigned int curStation = 0;   //index for current selected station in stationlist
unsigned int actStation = 0;   //index for current station in station list used for streaming 
unsigned long lastchange = 0;  //time of last selection change
unsigned int maxVol = 50; //maximale Einstellung Lautstärke
unsigned int curVol;      //gespeicherte Lautstärke   
bool btnStation = false;  //Taster "Fav"
bool rotaryVol = true;    //Encoder betätigt Lautstärke
bool streamReady = false; //Stream läuft
bool btnMode = false;     //Taste "Mode" gedrückt
bool btnStandby = false;     //Taste Standby gedrückt
time_t now;                          // this are the seconds since Epoch (1970) - UTC
tm tm;                             // the structure tm holds time information in a more convenient way *
unsigned long delayTimeRefresh;

//OTA Programme
void onOTAStart()
{
  Serial.println("OTA Update startet");
  audio.stopSong();
  lcd.clear();
  lcd.home();
  lcd.print("Firmware-Update");
}
void onOTAEnd(bool success)
{
  if (success)
  {
    Serial.println("Update erfolgreich");
    lcdPrint(0, 1, "fertig, Neustart");
    delay(3000);
    ESP.restart();
  }
  else
  {
    Serial.println("Update fehlerhaft");
    lcdPrint(0, 1, "  !!!Fehler!!!");
    delay(3000);
    ESP.restart();
  }
}

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
    WiFi.begin(SSID, PSK);
     while ((WiFi.status() != WL_CONNECTED) && (tryCount<MAXWLANTRY)) {
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



//setup
void setup() 
{
  //I2C Init
  Wire.begin(SDA,SCL, 100000);
  //Init Serial
  Serial.begin(115200);
  
  curStation = 0;
  curVol = 2;
  //start preferences instance
  pref.begin("radio", false);
  //set current station to saved value if available
  if (pref.isKey("station")) curStation = pref.getUShort("station");      //EEPROM Station lesen
  if (pref.isKey("volume")) curVol = pref.getUShort("volume");      //EEPROM volume lesen
  if (pref.isKey("standby")) btnStandby = pref.getBool("standby");      //EEPROM Standby lesen
  Serial.printf("Gespeicherte Lautstärke %i\n",curVol);
  Serial.printf("Gespeicherte Station %i von %i\n",curStation,STATIONS);
  if (curStation >= STATIONS) curStation = 0;
  //set active station to current station 
  actStation = curStation;
  
  //init Rotary / Buttons
  setup_rotary();
  pinMode (favButton, INPUT_PULLUP);
  pinMode (modeButton, INPUT_PULLUP);
  pinMode (standbyButton, INPUT_PULLUP);
  
   //Setup Audio
  setup_audio();
  
  //Setup LCD
  lcd.init();
  lcd.backlight();
  //init the special Symbol for LCD
  lcd.createChar(1, speaker);
  lcd.createChar(2, fullspeaker);

  //Setup Timeserver
  // ESP32 seems to be a little more complex:
  configTime(0, 0, NTP_SERVER);  // 0, 0 because we will use TZ in the next line
  setenv("TZ", MY_TZ, 1);            // Set environment variable with your time zone
  tzset();
  
  //init Wifi
  while (!setup_wifi()) 
  {
    Serial.println("Cannot connect :(");
    lcd.clear();
    lcdPrint(1, 0, "WLAN nicht da..");
    delay(3000);
    ESP.restart();
  }
  //init Server
  server.on("/", []()
  {
    server.send(200, "text/plain", "Webradio Update, 'IP-Adresse'/update eingeben");
  });
  //init OTA
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP Server startet");

  if (!(btnStandby))
  {
    //show on display and start streaming setEncoderValue(curStation);
    startUrl();
    showStation();  
  }
  else
  {
    lcd.clear();
    showStandby();
  }
  delayTimeRefresh = millis();
}

void loop() {
  if (!(btnStandby))
  {
    audio_loop();
    rotary_loop();
  }
  else        //Standby gewählt
  {
    if ((millis() - delayTimeRefresh) > 1000)     //jede Sekunde Aktualisierung der Uhrzeit und Datum
    {
      delayTimeRefresh = millis();
      showStandby();
    }
  }
  server.handleClient();
  ElegantOTA.loop(); 
  //read events from buttons
  button_loop();
}
