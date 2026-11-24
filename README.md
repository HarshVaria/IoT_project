# VoltGuard

**Physics-informed edge detection of battery sensor attacks on ESP32**

VoltGuard is an ESP32 battery sensor anomaly-detection prototype. It reads voltage and current through an INA219 and surface temperature through an NTC thermistor. Firmware samples at 10 Hz, forms a 600-sample (60-second) window, computes 33 physics-informed and statistical features, and classifies normal operation or one of five software-injected sensor attack patterns using a compact embedded Random Forest.

A separate FreeRTOS task connects to Firebase Realtime Database for commands, telemetry, and prediction results. Cloud connectivity is used for monitoring; inference runs locally on the ESP32.

## Repository contents

- `firmware/` - ESP32 sketch and C/C++ modules for sensing, feature extraction, attack injection, model inference, and Firebase I/O.
- `VoltGuard_Project_Report.pdf` - four-page report focused on the design, novelty, implementation, evaluation, and limitations.
- `images/` - prototype and system-architecture images.

The short demonstration video is kept separate from this code repository. Add its Drive/YouTube link here if you want the video discoverable from GitHub.

## Hardware

- ESP32-WROOM-32
- INA219 voltage/current sensor (I2C SDA GPIO 21, SCL GPIO 22)
- 10 kOhm NTC thermistor (ADC GPIO 34)
- IRLZ44N MOSFET discharge control (GPIO 25)
- Single 18650 cell and resistive load

## Build notes

1. Open `firmware/esp32_firebase_dt_hardware.ino` in Arduino IDE.
2. Install the ESP32 Arduino core, Firebase ESP Client, Adafruit INA219 library, and emlearn runtime dependencies.
3. Set Wi-Fi and Firebase configuration locally. The repository contains placeholders; do not commit credentials or private keys.
4. Confirm Firebase authentication and database rules before connecting the device.
5. Select ESP32-WROOM-32, compile, and flash.

## Evaluation scope

The reported validation uses NASA B0005 discharge profiles and software-generated attacks. It is a prototype evaluation, not field validation or safety certification. The reported 2.83 ms processing time begins after a full 60-second window has been collected; it is not end-to-end detection delay.
