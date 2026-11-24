// ============================================================
// sensor_reader.cpp — Hardware Sensor Interface (INA219 + NTC)
// Battery Digital Twin - ESP32 Attack Detector
// ============================================================
#include "sensor_reader.h"
#include <Arduino.h>
#include <math.h>

SensorReader::SensorReader() : _ready(false), _startTime(0) {
    // constructor
}

void SensorReader::begin() {
    Wire.begin(SDA_PIN, SCL_PIN);
    
    if (!_ina219.begin()) {
        Serial.println("Failed to find INA219 chip");
        // Don't set _ready to true if it fails
    } else {
        Serial.println("[OK] INA219 Initialized.");
        _ready = true;
    }
    
    // NTC ADC setup
    pinMode(NTC_ADC_PIN, INPUT);
    // ESP32 ADC attenuation (typically needed for 0-3.3V range)
    analogSetPinAttenuation(NTC_ADC_PIN, ADC_11db);
    
    _startTime = millis();
}

bool SensorReader::isReady() {
    return _ready;
}

SensorSample SensorReader::read() {
    SensorSample sample;

    // Time
    sample.timestamp_sec = (millis() - _startTime) / 1000.0f;

    if (_ready) {
        // Voltage
        sample.voltage = _ina219.getBusVoltage_V();
        
        // Current (A) - convert from mA
        // We negate it to make discharge current negative, matching typical battery datasets
        // If your wiring gives negative naturally, remove the minus sign.
        sample.current = -(_ina219.getCurrent_mA() / 1000.0f); 
    } else {
        sample.voltage = 0;
        sample.current = 0;
    }

    // Temperature
    sample.temperature = readNTC();

    return sample;
}

float SensorReader::readNTC() {
    int analogValue = analogRead(NTC_ADC_PIN);
    
    // Prevent divide by zero
    if (analogValue == 0) return -273.15f; 
    
    // ESP32 ADC is 12-bit (0-4095)
    // Circuit: 3.3V -> 10k -> ADC_PIN -> NTC -> GND
    // Vadc = 3.3 * (R_NTC / (10k + R_NTC))
    // R_NTC = 10k / ((4095 / ADC) - 1)
    
    float resistance = NTC_SERIES_R / ((4095.0f / analogValue) - 1.0f);
    
    // Beta Equation
    // 1/T = 1/T0 + (1/B) * ln(R/R0)
    float steinhart;
    steinhart = resistance / NTC_NOMINAL_R;       // (R/Ro)
    steinhart = log(steinhart);                   // ln(R/Ro)
    steinhart /= NTC_B_COEFFICIENT;               // 1/B * ln(R/Ro)
    steinhart += 1.0f / (NTC_NOMINAL_T + 273.15f); // + (1/To)
    steinhart = 1.0f / steinhart;                 // Invert
    steinhart -= 273.15f;                         // Convert absolute temp to C
    
    return steinhart;
}
