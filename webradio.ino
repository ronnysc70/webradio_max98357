#include <WiFi.h> 
#include <Arduino.h>
//library for LCD display
#include <LiquidCrystal_I2C.h>
//esp32 library to save preferences in flash
#include <Preferences.h>

//I2C def
#define SDA 42
#define SCL 41

//#define heltec
#define ESP32S3Dev
//WLAN access fill with your credentials
#define SSID "luke skyrouter"
#define PSK "#ronny1712"
#define MAXWLANTRY 10  // try to connect with stored credentials MAXWLANTRY times
int tryCount = 0;

//Buttons
#define favButton 21
unsigned long buttonTimeStamp = 0;
unsigned long buttonPressedTime = 0;

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

//instance for LCD display
//Pins heltec:  SDA: 17, SCL: 18
//Pins ESP32S3: SDA: 42, SCL: 41
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//Special character to show a speaker icon for current station
uint8_t speaker[8]  = {0x03,0x05,0x19,0x11,0x19,0x05,0x03};
uint8_t fullspeaker[8] = {0x03,0x07,0x1F,0x1F,0x1F,0x1F,0x07,0x03};

//global variables
unsigned long delayTimeStamp = 0;   //Zeitstempel zur Berechnung Verzögerungen
uint8_t curStation = 0;   //index for current selected station in stationlist
uint8_t actStation = 0;   //index for current station in station list used for streaming 
uint32_t lastchange = 0;  //time of last selection change
unsigned int maxVol = 64; //maximale Einstellung Lautstärke
unsigned int curVol;      //gespeicherte Lautstärke   
bool btnStation = false;  //Taster "Fav"
bool rotaryVol = true;    //Encoder betätigt Lautstärke
bool streamReady = false; //Stream läuft


//Meldungen einzelne Zeile
int lcdPrint(int x, int y, const char *msg) 
{
  lcd.setCursor(x, y);
  lcd.print(msg);
  return 0;
}

//show name of current station on LCD display
//show the speaker symbol in front if current station = active station
void showStation() 
{
  int loff=0;
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
  lcd.setCursor(loff,0);
  lcd.print(curStation+1);
  if (curStation > 8) {
    lcd.setCursor(loff+2,0);
  } else {
    lcd.setCursor(loff+1,0);
  }
  lcd.print(":");
  String name = String(stationlist[curStation].name);
  if (name.length() < 12-loff)
    lcd.print(name);
  else {
    uint8_t p = name.lastIndexOf(" ",15); //if name does not fit, split line on space
    lcd.print(name.substring(0,p));
    lcd.setCursor(0,1);
    lcd.print(name.substring(p+1,p+17));
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
    lcd.clear();
    lcd.home();
    lcd.print("IP-Adresse:");
    lcdPrint(0, 1, WiFi.localIP().toString().c_str());
    delay(2000);
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
  Serial.printf("Gespeicherte Lautstärke %i\n",curVol);
  Serial.printf("Gespeicherte Station %i von %i\n",curStation,STATIONS);
  if (curStation >= STATIONS) curStation = 0;
  //set active station to current station 
  actStation = curStation;
  
  //init Rotary / Buttons
  setup_rotary();
  pinMode (favButton, INPUT_PULLUP);
 
  //Setup LCD
  lcd.init();
  lcd.backlight();
  //init the special Symbol for LCD
  lcd.createChar(1, speaker);
  lcd.createChar(2, fullspeaker);
  
  //init Wifi
  while (!setup_wifi()) 
  {
    Serial.println("Cannot connect :(");
    lcd.clear();
    lcdPrint(1, 0, "WLAN nicht da..");
    delay(1000);
  }
  //Setup Audio
  setup_audio();
    
  //show on display and start streaming setEncoderValue(curStation);
  startUrl();
  showStation();
  delayTimeStamp = millis();
  
}

void loop() {
  
  audio_loop();
  
  //read events from rotary encoder
  rotary_loop();
}
