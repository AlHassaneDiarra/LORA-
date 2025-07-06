#include "Temperature.h"

namespace temp_sensor {
    bool TemperatureSensor::isSetup = false;
    Adafruit_BME280 TemperatureSensor::bme;

    void TemperatureSensor::setup(){
        if (!TemperatureSensor::bme.begin(0x77, I2C)){
            Serial.println("Could not find a valid BME280 sensor");
            while(1) delay(10);
        }

        temperatureSensor = bme.getTemperatureSensor();
        pressureSensor = bme.getPressureSensor();
        humiditySensor = bme.getHumiditySensor();

        Serial.println("Setting up the BME280 sensor...");
        temperatureSensor->printSensorDetails();
        pressureSensor->printSensorDetails();
        humiditySensor->printSensorDetails();
    }

    TemperatureSensor::TemperatureSensor(TwoWire* i2c){
        this->I2C = i2c;
        if (!TemperatureSensor::isSetup){
            setup();
        }
    }

    float TemperatureSensor::getTemperature(){
        sensors_event_t event;
        temperatureSensor->getEvent(&event);
        return event.temperature;
    }

    float TemperatureSensor::getHumidity(){
        sensors_event_t event;
        humiditySensor->getEvent(&event);
        return event.relative_humidity;
    }

    float TemperatureSensor::getPressure(){
        sensors_event_t event;
        pressureSensor->getEvent(&event);
        return event.pressure;
    }

    TemperatureData TemperatureSensor::getTemperatureData(){
        TemperatureData data;
        getTemperatureData(&data);
        return data;
    }

    void TemperatureSensor::getTemperatureData(TemperatureData* data){
        data->temperature = getTemperature();
        data->humidity = getHumidity();
        data->pressure = getPressure();
    }
}