// Base libraries
#include <Arduino.h>
#include <heltec.h>

// Sensor libraries
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <Temperature.h>

#define LORA_BAND 868E6
#define LORA_ADDRESS (byte)0x2
#define LORA_WEB_ADDRESS (byte)0x1

// I2C
# define SDA 4
# define SCL 15
TwoWire I2C = TwoWire(1);

// Temperature sensor
using temp_sensor::TemperatureSensor;
using temp_sensor::TemperatureData;

TemperatureSensor* mySensor;
String loraMessage;
bool newMessage = false;
long messageId = 0;

int lastTempUpdate = 0;

void onReceive(int packetSize){
    Serial.println("✅ Message received");
    if (packetSize == 0) return;
    // Destination
    int recipient = LoRa.read();
    // Source
    byte sender = LoRa.read();
    // Message length
    byte incomingLength = LoRa.read();

    String incoming = "";

    while (LoRa.available()) incoming += (char)LoRa.read();


    // if (incomingLength != incoming.length()){
    //     return;
    // }

    // Si le message nous est pas destiner ou qu'il ne provient pas de l'address configurer
    // if ((recipient != LORA_ADDRESS && recipient != 0xFF) || sender != LORA_WEB_ADDRESS){
    //     return;
    // }

    loraMessage = incoming;
    newMessage = true;
}

void setup(){
    // Serial output setup
    Serial.begin(115200);

    // I2C for the temperature sensor
    
    // Heltec/LoRa setup
    Heltec.begin(true, true, true, true, LORA_BAND);
    LoRa.begin(LORA_BAND, true);
    LoRa.onReceive(onReceive);
    LoRa.receive();
    
    I2C.begin(SDA, SCL);


    // Temperature sensor
    mySensor = new TemperatureSensor(&I2C);

    // Builtin LED setup
    pinMode(LED_BUILTIN, OUTPUT);

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

void sendTemperatureData(TemperatureData data){
    String message = String(data.temperature) + ";" + data.humidity + ";" + data.pressure;
    sendMessage(LORA_WEB_ADDRESS, message);
}

void updateDisplay(){
    Heltec.display->clear();
    Heltec.display->flush();
    Heltec.display->resetOrientation();
    Heltec.display->drawString(0,0, ((String("Last message: \n") + loraMessage)).c_str());
    Heltec.display->display();
}

void loop(){
    if (millis() - lastTempUpdate >= 1000){
        Serial.println("Reading sensor data...");
        TemperatureData data;
        mySensor->getTemperatureData(&data);
        sendTemperatureData(data);
        lastTempUpdate = millis();
    }

    if(newMessage){
        Serial.print("Niveau Minimum 🔥: ");
        Serial.println(loraMessage);
        updateDisplay();
        newMessage = false;
    }
    delay(10);
}