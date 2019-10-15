#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#else
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <FS.h>
#endif
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>


#ifdef ESP8266
#define WiFiMulti ESP8266WiFiMulti
#define WebServer ESP8266WebServer

#else 
#define WiFiMulti WiFiMulti
#define WebServer WebServer
#define PWMRANGE 255
#define Dir File
#define openDir open 
#endif

WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

WebServer server(80);       // create a web server on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

File fsUploadFile;                                    // a File variable to temporarily store the received file

const char *ssid = "ESP8266 Access Point"; // The name of the Wi-Fi network that will be created
const char *password = "thereisnospoon";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "bababubu";           // A name and a password for the OTA service
const char *OTAPassword = "somethingnotsocommon";

#define DIM_PIN 13
#define POWERON_PIN 15

const char* mdnsName = "esp8266"; // Domain name for the mDNS responder

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
 #if defined(ARDUINO_ARCH_ESP32)
	ledcAttachPin(m_nPinR, m_nChannelR);
  ledcSetup(DIM_PIN, OUTPUT);
	#else
  pinMode(DIM_PIN, OUTPUT);    // the pins with LEDs connected are outputs
	#endif 
  pinMode(POWERON_PIN, OUTPUT);

  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  
  startOTA();                  // Start the OTA service
  
  startSPIFFS();               // Start the SPIFFS and list all contents

  startWebSocket();            // Start a WebSocket server
  
  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler
  
}

/*__________________________________________________________LOOP__________________________________________________________*/

bool LCDPowerOn = false; // LCD power state. True: on false: off 
int brightness = 0; // brightness value that is set on LCD panel
bool looping_effect = false; // if true loop LCD brightness from max to min and repeat til false 
int interval = 1; // interval for looping effect

unsigned long prevMillis = millis();
bool rising = true;
int stepsize = 10;
double brightness_slope=0;

void loop() {
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  ArduinoOTA.handle();                        // listen for OTA events

  // execute looping effect
  // turn the brightness to max and min in the specified interval
  if(looping_effect) {  
    if(millis() > prevMillis + stepsize) { // stepsize defines the the resolution! For short stepsizes better use interrupts instead of blocking like this   
      //Serial.println("###########################");
      //Serial.printf("pwmrange:%i; stepsize: %i; interval: %i\n",PWMRANGE, stepsize, interval);
      //Serial.printf("pwmrange*stepsize=%i*%i=%i\n",PWMRANGE, stepsize, PWMRANGE*stepsize);
      //Serial.printf("interval/2: %i\n",(double)interval/2);

    // the brightness slope defines the value by which the brightness is in-/decreased each cycle
      brightness_slope=(double)((PWMRANGE*stepsize)/(interval*1000));

      //Serial.print("pwmrange*2*stepsize/(interval)=");
      //Serial.println(brightness_slope);

      //Serial.println("rising? "+(rising)?"Yes!":"No!");

    // check if brightness is in valid borders 
    //  1.) >= max allowed brightness 
    //  2.) not negative
    // and set if brightness should rise, bc min was hit or decrease, bc max was hit 
      if(brightness >= PWMRANGE) 
	rising=false;
      if(brightness <= 0)
	rising=true;

    // in terms of rising in-/decrease brightness value by slope
      brightness+=(rising)?brightness_slope:-brightness_slope;
      //Serial.print("Brightness: ");
      //Serial.println(brightness);
      setLCDBrightness(); 
  
      prevMillis = millis();
    }
  }
}

/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");

  wifiMulti.addAP("NETGEAR_AC1750", "1667+came+INCREASE+each+moon+dare+NEITHER+GENERAL+FINLAND+6303~");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)

  Serial.println("SPIFFS started");
 #ifdef ESP8266 
  Serial.println(" Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
  else 
  Serial.println(" Contents:");
  File root = SPIFFS.open("/");
 
  File file = root.openNextFile();
 
  while(file){
 
      Serial.print("FILE: ");
      Serial.println(file.name());
 
      file = root.openNextFile();
  }
  #endif
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", ""); 
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
                                              // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

/**
* if the requested file or page doesn't exist, return a 404 not found error
*/
void handleNotFound(){ 
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

/**
* if file exists return true and send the file to the client, else return false
* @return true if file exists, false if not
*/
bool handleFileRead(String path) { 
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

/**
* upload a new file to the SPIFFS
*/
void handleFileUpload(){ 
  HTTPUpload& upload = server.upload();
  String path;
  if(upload.status == UPLOAD_FILE_START){
    path = upload.filename;
    if(!path.startsWith("/")) path = "/"+path;
    if(!path.endsWith(".gz")) {                          // The file server always prefers a compressed version of a file 
      String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
      if(SPIFFS.exists(pathWithGz))                      // version of that file must be deleted (if it exists)
         SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      //looping_effect = false;                  // Turn looping off when disconnecting 
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        //looping_effect = false;                  // Turn looping off when a new connection is established
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
      if (payload[0] == 'o') {            
        LCDPowerOn = true; // set LCD power state to on 
        digitalWrite(POWERON_PIN, 1);
  	Serial.println("Turn on LCD");
      } else if (payload[0] == 'p') {            
        Serial.println("Turn off LCD");
        LCDPowerOn = false; // set LCD power state to off 
        digitalWrite(POWERON_PIN, 0);
      } else if (payload[0] == 'b') {             
        brightness = atoi((const char *) &payload[1]); // decode string to int 
        setLCDBrightness(); 
      } else if (payload[0] == 'e') { // enable looping_effect
        looping_effect = true;
  	Serial.println("enable looping");
      } else if (payload[0] == 'd') { // enable looping_effect 
        looping_effect = false;
  	Serial.println("disable looping");
      } else if (payload[0] == 'i') {
        interval = atoi((const char *) &payload[1]);   // decode string to int 
      }
    break;
  }
}

/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

/**
* convert sizes in bytes to KB and MB
*/
String formatBytes(size_t bytes) { 
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

/**
* determine the filetype of a given filename, based on the extension
*/
String getContentType(String filename) { 
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

/**
* set the brightness off the LCD panel
*/
void setLCDBrightness(){ 
  if(LCDPowerOn){
    if(brightness>PWMRANGE){
      brightness=PWMRANGE;
    }
  }else{
    //brightness=-1; //override brightness value - not nessassary
    Serial.printf("Will set brightness value, but FYI the LCD is turned off - set LCDPowerOn to turn the LCD on!\n");
  }
  #if defined(ARDUINO_ARCH_ESP32)
			ledcWrite(DIM_PIN,brightness);
		#else
  analogWrite(DIM_PIN,brightness);                         // write to output pin
		#endif
  webSocket.broadcastTXT("b: "+String(brightness));
  Serial.printf("Brightness value: %i\n",brightness);
}   
