# VoltGuard - ESP32 battery sensor attack detector

This submission contains ESP32 firmware modules for battery sensing, feature extraction, controlled attack injection, embedded Random Forest inference, and Firebase telemetry.

## Hardware
- ESP32-WROOM-32
- INA219 voltage/current sensor (I2C SDA 21, SCL 22)
- 10 kOhm NTC thermistor (ADC GPIO 34)
- MOSFET discharge control (GPIO 25)
- 18650 cell and resistive load

## Firmware setup
1. Open `esp32_firebase_dt_hardware.ino` in Arduino IDE.
2. Replace Wi-Fi and Firebase placeholders with project-specific values kept outside public source control.
3. Confirm Firebase database rules and authentication for your deployment.
4. Install the ESP32 Arduino core, Firebase ESP Client, Adafruit INA219, and emlearn runtime dependencies.
5. Select ESP32-WROOM-32 and compile/upload.

The detector samples at 10 Hz, processes 600-sample (60-second) windows, and extracts 33 features before local classification. Cloud telemetry runs separately from edge inference. Evaluation attacks are injected in software and do not represent live bus intrusion validation.
