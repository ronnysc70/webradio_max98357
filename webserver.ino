void setup_webserver() {
  //server.enableCORS(); 
  server.on("/",handleRoot);
  //handle ajax commands
  server.on("/cmd/stations",sendStations);
  server.on("/cmd/restorestations",restoreStations);
  server.on("/cmd/restart",restart);
  server.on("/cmd/getstation",getStationData);
  server.on("/cmd/setstation",setStationData);
  server.on("/cmd/teststation",testStation);
  server.on("/cmd/endtest",endTest);
  //start webserver
  server.begin();
}

void webserver_loop() {
    server.handleClient();
}

//handle root request
void handleRoot() {

    //if connected send the main page
    server.send(200,"text/html",MAIN_page);
 


}

//AJAX command /cmd/stations
void sendStations() {
  char * s;
  char * sel;
  char buf[100];
  //prepare HTML response
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  //send all entries as an option list
  for (uint8_t i = 0; i<STATIONS; i++) {
    //add selected if the station is tthe current one
    sel =  "";
    if (i == actStation) sel = "selected";
    //set a black dot in front of station name if active
    s = "&#x2002;";
    if (stationlist[i].enabled) s="&#x25cf;";
    //create the option entry with the template
    sprintf(buf,OPTION_entry,i,sel,s,stationlist[i].name);
    server.sendContent(buf);
  }
  
}



//AJAX command /cmd/getstation
void getStationData() {
  //expects one argument with the stationid
  if (server.hasArg("stationid")) {
    uint8_t i = server.arg("stationid").toInt();
    if (i>=STATIONS) i = STATIONS-1;
    //build message string with station name, station url and enabled flag. Separated by new line
    String msg = String(stationlist[i].name) + "\n" + String(stationlist[i].url) + "\n" + String(stationlist[i].enabled) + "\n" + String(i+1);
    server.send(200,"text/plain",msg);
  } else {
    server.send(200,"text/plain","Invalid command");
  }
}

//AJAX command /cmd/setstation
void setStationData() {
  //expects an argument with a valid station id
  //data are sent as arguments too
  //name = the station name
  //url = the station url
  //enabled = enabled flag
  //position = new position in list
  char key[5];
  if ((server.hasArg("stationid")) && (server.arg("stationid").toInt() < STATIONS))  {
    uint8_t i = server.arg("stationid").toInt();
    //The values from the arguments will be saved in the station list
    if (server.hasArg("name")){
      sprintf(key,"n%i",i);
      sender.putString(key,server.arg("name"));
      strlcpy(stationlist[i].name,server.arg("name").c_str(),32);
    }
    if (server.hasArg("url")){
      sprintf(key,"u%i",i);
      sender.putString(key,server.arg("url"));
      strlcpy(stationlist[i].url,server.arg("url").c_str(),150);
    }
    if (server.hasArg("enabled")){
      sprintf(key,"f%i",i);
      sender.putUChar(key,server.arg("enabled").toInt());
      stationlist[i].enabled = server.arg("enabled").toInt();    
    }
    if (server.hasArg("position")){
      int16_t newpos = server.arg("position").toInt();
      newpos--;
      Serial.printf("Move %i to position %i\n",i+1,newpos+1);
      if ((i != newpos) && (newpos >= 0) && (newpos < STATIONS)) {
        reorder(i,newpos);
        saveList();
      }
    }
    server.send(200,"text/plain","OK");
  } else {
    server.send(200,"text/plain","Invalid station ID");
  }
}

//AJAX command /cmd/teststation
void testStation() {
  //exspects one argument with the url to be tested
  bool ret = true;
  if (server.hasArg("url"))  {
    ret = audio.connecttohost(server.arg("url").c_str());
  }
  if (ret) {
    //if success respond is OK
    server.send(200,"text/plain","OK");
  } else {
    //if no success switch back to the current station
    //and respond with ERROR
    audio.connecttohost(stationlist[actStation].url);
    server.send(300,"text/plain","ERROR");
  }
}

//AJAX command /cmd/endtest
void endTest() {
  //switch back to the current station to end the test
  audio.connecttohost(stationlist[actStation].url);
  //respond with OK
  server.send(200,"text/plain","OK");
}

void restoreStations() {
  restore();
  //respond with OK
  server.send(200,"text/plain","OK");
}
//AJAX command /cmd/restart
void restart(){
  ESP.restart();
}
