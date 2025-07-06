#ifndef TEMPERATURE_H
#define TEMPERATURE_H
#include <Adafruit_BME280.h>

namespace temp_sensor{

    struct TemperatureData {
        float temperature;
        float humidity;
        float pressure;
    };

    class TemperatureSensor{
        private:
            static bool isSetup;
            static Adafruit_BME280 bme;
            TwoWire* I2C;
            Adafruit_Sensor* temperatureSensor;
            Adafruit_Sensor* pressureSensor;
            Adafruit_Sensor* humiditySensor;

            void setup();

        public:
            TemperatureSensor(TwoWire* i2c);
            float getTemperature();
            float getHumidity();
            float getPressure();
            TemperatureData getTemperatureData();
            void getTemperatureData(TemperatureData* data);


    };
}

#endif /* TEMPERATURE_H */