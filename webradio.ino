#include <WiFi.h>
#include <WebServer.h>
#include <NetworkClientSecure.h>
#include <ElegantOTA.h> 
#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include <Bounce2.h>

NetworkClientSecure httpsClient;

/* Configuration of NTP */
// choose the best fitting NTP server pool for your country
#define NTP_SERVER "de.pool.ntp.org"

// choose your time zone from this list
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"

//home page and template for options
#include "index.h"
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
#include "Audio.h" //see repository at github "https://github.com/schreibfaul1/ESP32-audioI2S"
// Digital I/O used

#define I2S_DOUT      4
#define I2S_BCLK      5
#define I2S_LRC       6

Audio audio;

//WiFI Einstellungen in der credentials.h
#include <credentials.h>
#define MAXWLANTRY 10  // try to connect with stored credentials MAXWLANTRY times
int tryCount = 0;

//Button Einstellungen
const uint8_t button_pins[] = {38, 47, 21}; // button pins 42 = Standby, 41 = Mode, 40 = Fav
#define NUMBUTTONS sizeof(button_pins)
Bounce * buttons = new Bounce[NUMBUTTONS];

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
  char url[150];  //stream url
  char name[32];  //stations name
  uint8_t enabled;//flag to activate the station
} Station;
#define STATIONS 20 //number of stations in the list

//gloabal variables
Station stationlist[STATIONS];

//instance of prefernces
Preferences pref;
Preferences sender;       //für Senderliste

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
bool internetLost = false; //Internet DNS nicht erreichbar
bool updateIsRunning = false;   //Update läuft
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
  updateIsRunning = true;
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
  sender.begin("senderlist",false);
	//set current station to saved value if available
	if (pref.isKey("station")) curStation = pref.getUShort("station");
	if (curStation >= STATIONS) curStation = 0; //check to avoid invalid station number
  if (pref.isKey("volume")) curVol = pref.getUShort("volume");      //EEPROM volume lesen
  if (pref.isKey("standby")) btnStandby = pref.getBool("standby");      //EEPROM Standby lesen
  Serial.printf("Gespeicherte Lautstärke %i\n",curVol);
  Serial.printf("Gespeicherte Station %i von %i\n",curStation,STATIONS);
  
  //set active station to current station 
  actStation = curStation;
  if (curVol < 10)
  {
    curVol = 10;
  }
  //init Rotary / Buttons
  setup_rotary();
  
  // Make input & enable pull-up resistors on pushbuttons
  for (int i=0; i<NUMBUTTONS; i++) 
  {
    buttons[i].attach( button_pins[i], INPUT_PULLUP); // setup the bounce instance for the current button
    buttons[i].interval(50); // interval in ms
  }
  
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
  setup_senderList(); //load station list from preferences
	setup_webserver();
  Serial.println("Webserver läuft");
  //init OTA
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onEnd(onOTAEnd);

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
  httpsClient.setInsecure();
}

void loop() 
{
  if (!(updateIsRunning))
  {
    if (!(btnStandby))
    {
      audio_loop();
      rotary_loop();
      if (!(btnStation))        //wenn Senderwahl keine Abfrage des Wifi
      {
        wifi_loop();
      }
    }
    else        //Standby gewählt
    {
      if ((millis() - delayTimeRefresh) > 1000)     //jede Sekunde Aktualisierung der Uhrzeit und Datum
      {
        delayTimeRefresh = millis();
        showStandby();
      }
    }
    //read events from buttons
    button_loop();
  }  
  //Check for http requests
	webserver_loop();
  ElegantOTA.loop(); 
}
