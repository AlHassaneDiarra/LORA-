/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-lora-sensor-web-server/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include "heltec.h"

// Import Wi-Fi library
#include <WiFi.h>
#include "ESPAsyncWebServer.h"

#include <SPIFFS.h>

//Libraries for LoRa
#include <SPI.h>
//#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Libraries to get time from NTP Server
#include <NTPClient.h>
#include <WiFiUdp.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 868E6
#define LORA_ADDRESS (byte)0x1
#define LORA_SENS_ADDRESS (byte)0x2

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Requete pour la temperature min
#define TEMPERATURE_MIN "TemperatureMin"

// Replace with your network credentials
const char* ssid = "CLC";
const char* password = "Bonjour!";
//const char* ssid     = "VIDEOTRON4482";
//const char* password = "4KHM994PJY33Y";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String day;
String hour;
String timestamp;


// Initialize variables to get and save LoRa data
int rssi;
int temperatureMinThreshold = 0;
String loraMessage;
String loraMessageOut;
bool newMessage = false;
bool newMessageOut = false;
bool alertHigh = false;             // sera à true si temp > seuil
struct TemperatureData {
  float temperature;
  float humidity;
  float pressure;
} temperatureData;

String readingID;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(temperatureData.temperature);
  }
  else if(var == "HUMIDITY"){
    return String(temperatureData.humidity);
  }
  else if(var == "PRESSURE"){
    return String(temperatureData.pressure);
  }
  else if(var == "TIMESTAMP"){
    return timestamp;
  }
  else if (var == "RRSI"){
    return String(rssi);
  }
  return String();
}

//Initialize OLED display
void startOLED(){
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA SENDER");
}

void connectWiFi(){
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.setCursor(0,20);
  display.print("Access web server at: ");
  display.setCursor(0,30);
  display.print(WiFi.localIP());
  display.display();
}

// Function to get date and time from NTPClient
void getTimeStamp() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedTime();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  day = formattedDate.substring(0, splitT);
  // Serial.println(day);
  // Extract time
  hour = formattedDate.substring(splitT+1, formattedDate.length()-1);
  // Serial.println(hour);
  timestamp = day + " " + hour;
}

void decodeMessage(){
  int temperatureIndex = loraMessage.indexOf(';');
  int humidityIndex = loraMessage.indexOf(';', temperatureIndex + 1);
  int pressureIndex = loraMessage.indexOf(';', humidityIndex + 1);
  String temperatureStr = loraMessage.substring(0, temperatureIndex);
  String humidityStr = loraMessage.substring(temperatureIndex + 1, humidityIndex);
  String pressureStr = loraMessage.substring(humidityIndex + 1, pressureIndex);

  temperatureData.temperature = temperatureStr.toFloat();
  temperatureData.humidity = humidityStr.toFloat();
  temperatureData.pressure = pressureStr.toFloat();
  
  Serial.printf("Temperature: %.2f\nHumidity: %.2f\nPressure: %.2f\n", 
  temperatureData.temperature, 
  temperatureData.humidity, 
  temperatureData.pressure);

  // Détection d’alerte
if (temperatureData.temperature > temperatureMinThreshold) {
  alertHigh = true;
  digitalWrite(BUILTIN_LED, HIGH);   // allume la LED pour alerter
} else {
  alertHigh = false;
  digitalWrite(BUILTIN_LED, LOW);    // éteint la LED
}

  Serial.print("With RSSI ");
  Serial.println(rssi);
  rssi = LoRa.packetRssi();
}

void sendMessage(byte dest, String message){
  digitalWrite(BUILTIN_LED, HIGH);
  LoRa.beginPacket();
  LoRa.write(dest);
  LoRa.write(LORA_ADDRESS);
  LoRa.write(message.length());
  LoRa.print(message);
  LoRa.endPacket();
  digitalWrite(BUILTIN_LED, LOW);
  LoRa.receive();
}

void onReceive(int packetSize){
  digitalWrite(BUILTIN_LED, HIGH);
  if (packetSize == 0) return;

  int recipient = LoRa.read();
  byte sender = LoRa.read();
  byte incomingLength = LoRa.read();

  String incoming = "";

  while (LoRa.available()){
    incoming += (char)LoRa.read();
  }

  digitalWrite(BUILTIN_LED, LOW);
  if (incomingLength != incoming.length()){
    return;
  }

  // if (recipient != LORA_ADDRESS && recipient != 0xFF){
  //   return;
  // }

  loraMessage = incoming;
  newMessage = true;
}

void setupLoRa(){
  if(!LoRa.begin(BAND, true)){
    Serial.println("Lora setup failed...");
    while(1) delay(10);
  }
  LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa setup finished");
}

void setup() { 
  // Initialize Serial Monitor
  Serial.begin(115200);
  startOLED();
  setupLoRa();
  connectWiFi();
  pinMode(BUILTIN_LED, OUTPUT);

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/jquery-1.4.4.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/jquery-1.4.4.min.js", "text/javascript");
  });
  server.on("/smartslider.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/smartslider.js", "text/javascript");
  });
  server.on("/smartslider.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/smartslider.css", "text/css");
  });
  server.on("/gradient_red.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/gradient_red.jpg", "image/jpg");
  });
  server.on("/tracker_simple.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/tracker_simple.png", "image/png");
  });
  
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperatureData.temperature).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperatureData.humidity).c_str());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperatureData.pressure).c_str());
  });
  server.on("/timestamp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", timestamp.c_str());
  });
  server.on("/rssi", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(rssi).c_str());
  });
  server.on("/winter", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/winter.jpg", "image/jpg");
  });
  server.on("/setTemperatureAlarm", HTTP_GET, [](AsyncWebServerRequest *request){
  if (request->hasParam(TEMPERATURE_MIN)) {
    String v = request->getParam(TEMPERATURE_MIN)->value();
    temperatureMinThreshold = v.toInt();
    loraMessageOut = v;        
    newMessageOut = true;      
    request->send(200, "text/plain", v);
  } else {
    request->send(400, "text/plain", "Paramètre manquant");
  }
});  

  // Start server
  server.begin();
  
  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);
}

void loop() {
    if (newMessage){
      Serial.print("New message: ");
      Serial.println(loraMessage);
      // getTimeStamp();
      decodeMessage();
      newMessage = false;

      // --- NOUVEAU : alerte si on dépasse le seuil ---
      if (temperatureData.temperature > temperatureMinThreshold) {
        Serial.println("🔥Température supérieure au seuil !");
      }

      // ---------------------------------------------

      // envoi éventuel de la nouvelle consigne au capteur
      if (newMessageOut){
        Serial.printf("✅Envoi consigne TempMin = %d via LoRa\n", temperatureMinThreshold);
        sendMessage(LORA_SENS_ADDRESS, loraMessageOut);
        newMessageOut = false;
      }
    }
    delay(10);
}