// ============================================================
// sensor_reader.h — INA219 + NTC Sensor Interface
// Battery Digital Twin - ESP32 Attack Detector
// ============================================================
#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <Wire.h>
#include <Adafruit_INA219.h>
#include "config.h"

struct SensorSample {
    float voltage;        // Battery voltage (V)
    float current;        // Current (A), negative = discharge
    float temperature;    // Temperature (°C)
    float timestamp_sec;  // Time in seconds since monitoring started
};

class SensorReader {
public:
    SensorReader();
    void begin();
    SensorSample read();
    bool isReady();

private:
    Adafruit_INA219 _ina219;
    float readNTC();
    bool _ready;
    unsigned long _startTime;
};

#endif
