# VoltGuard

**Physics-informed edge detection of battery sensor attacks on ESP32**

VoltGuard is a battery-management-system (BMS) security prototype that detects manipulated or abnormal voltage, current, and temperature sensor data close to the device. An ESP32-WROOM-32 samples the sensors and classifies data locally with a compact Random Forest. A separate Firebase connection provides remote commands and telemetry without placing cloud communication in the inference path.

## Why this project

A fixed threshold can miss a plausible-looking sensor offset, replayed value, or gradual drift. VoltGuard combines signal statistics with battery-domain and cross-sensor consistency checks, so the classifier can use changes over time and relationships among voltage, current, and temperature. The design also measures whether this feature-and-model pipeline fits the memory and compute budget of an ESP32.

## System overview

The implementation has three connected parts:

1. **Physical sensing testbench** - a single 18650 cell, resistive discharge load, MOSFET switch, INA219 voltage/current sensor, NTC thermistor, and ESP32.
2. **Local edge detector** - sensor sampling, a 600-reading circular window, feature extraction, attack-injection path for controlled tests, and embedded inference.
3. **Cloud monitoring** - Firebase Realtime Database paths for commands, status, and prediction results, with a dashboard for live telemetry and alerts.

The dual-core firmware separates these responsibilities. Core 1 handles 10 Hz sensing, window preparation, feature extraction, and inference. Core 0 handles Firebase commands and telemetry. The tasks exchange state through shared data and synchronization flags, keeping network requests outside the sensing and inference path.

## Hardware and connections

| Component | Project configuration |
|---|---|
| ESP32-WROOM-32 | Dual-core Xtensa LX6 at 240 MHz; local feature processing and classification |
| INA219 | Voltage/current measurement over I2C; SDA GPIO 21, SCL GPIO 22, address 0x40 |
| NTC thermistor | 10 kOhm nominal resistance, Beta 3950 K; ADC1 GPIO 34 |
| MOSFET | IRLZ44N low-side discharge switch controlled by GPIO 25 |
| Cell and load | Single nominal 3.7 V 18650 cell with 5 Ohm, 10 W resistive load |

The thermistor divider reading is converted to resistance and temperature using the Beta relation. Discharge current is represented with a negative sign. Thermistor placement, calibration, and electrical noise can affect the measured surface temperature.

## Sampling and feature pipeline

- Sampling rate: **10 Hz**
- Window: **600 samples**, non-overlapping, or **60 seconds**
- Feature vector: **33 values**
- Output classes: normal operation plus **five injected attack patterns**

The feature vector combines three groups:

- **Physical-consistency features:** discharge direction, positive voltage-rate behavior, temperature-rate violations, voltage/current correlation, voltage and temperature bounds, and power statistics.
- **Signal statistics:** means, standard deviations, minima, maxima, ranges, slopes, and current magnitudes across voltage, current, and temperature.
- **Temporal and discontinuity features:** temperature-rate variation, lag-one autocorrelation, maximum voltage and temperature jumps, window duration, and estimated discharged energy.

### Attack patterns in the controlled evaluation

| Class | Injected signal behavior | Main detection cues |
|---|---|---|
| Temperature spike | Short temperature excursion | Temperature rate, maximum, and discontinuity |
| Voltage offset | Bias added to voltage readings | Voltage jump and voltage/current consistency |
| Gradual SOC drift | Slowly changing voltage bias | Discharge trend and positive voltage-rate fraction |
| Random thermal noise | Added temperature noise | Temperature-rate variation and autocorrelation |
| Time-delay replay | Voltage and temperature replayed with delay while current remains live | Cross-channel timing mismatch |

These patterns are software-injected into the sample buffer before feature calculation. They make evaluation repeatable but do not emulate a live compromise of the I2C bus or a vehicle network.

## Embedded model and resource profile

The Random Forest is trained in a Python/scikit-learn workflow and converted to C using emlearn. The firmware feature order must match the order used during training. The physical configuration uses the **10-tree** model; a 100-tree configuration is also profiled for comparison.

| Measurement on ESP32-WROOM-32 | 10 trees | 100 trees |
|---|---:|---:|
| Model footprint | 17.7 KB | 175.8 KB |
| Feature extraction | 2.45 ms | 2.45 ms |
| Inference | 0.38 ms | 3.82 ms |
| Total after window is ready | **2.83 ms** | **6.27 ms** |

The measured pipeline starts **after all 600 samples have been collected**. With non-overlapping 60-second windows, acquisition can add up to 60 seconds before classification, so the millisecond processing time is not the end-to-end detection delay. The 10-tree profile reports a 0.0047% pipeline duty cycle per 60-second window; this excludes sensing, Wi-Fi, operating-system activity, and other tasks.

## Evaluation summary

The evaluation uses NASA Ames B0005 18650 discharge profiles as a baseline (2.0 A, 4.2 V to 2.7 V, and 24 °C in the documented setup) with synthetic sensor attacks. The balanced validation set contains 500 windows per class across six classes, for 3,000 windows total.

| Class | Precision | Recall | F1 |
|---|---:|---:|---:|
| Normal | 0.932 | 0.941 | 0.936 |
| Temperature spike | 1.000 | 1.000 | 1.000 |
| Voltage offset | 0.929 | 0.920 | 0.924 |
| Gradual SOC drift | 0.953 | 0.960 | 0.956 |
| Random thermal noise | 1.000 | 1.000 | 1.000 |
| Time-delay replay | 0.940 | 0.938 | 0.939 |
| **Macro average** | **0.959** | **0.960** | **0.959** |

Voltage offset is the most difficult class in this evaluation, especially when a small bias resembles ordinary discharge variation. The validation results are a baseline for the specified synthetic setup, not proof of performance on an adversarial field deployment.

## Build and configure

1. Open `firmware/esp32_firebase_dt_hardware.ino` in Arduino IDE.
2. Install the ESP32 Arduino core, Firebase ESP Client, Adafruit INA219 library, and emlearn runtime dependency (`eml_common.h`).
3. Set `WIFI_SSID`, `WIFI_PASSWORD`, and Firebase configuration locally. The committed source uses placeholders; never commit credentials or private keys.
4. Confirm Firebase authentication and Realtime Database rules for your project.
5. Select ESP32-WROOM-32, compile, and flash.

The firmware uses Firebase paths for commands, status, and results (including the 33-feature vector). Remote control and telemetry are intended for monitoring and demonstrations; model inference executes locally.

## Repository contents

- `firmware/` - ESP32 sketch and C/C++ modules for sensor reading, feature extraction, attack injection, model inference, and Firebase I/O.
- `VoltGuard_Project_Report.pdf` - four-page implementation and evaluation report.
- `images/` - hardware demonstration, testbench close-up, and system architecture.

The short demonstration video is kept separate from the code repository.

## Scope and next steps

This is a proof-of-concept, not a complete or safety-certified automotive BMS. The evaluation uses one NASA cell baseline and synthetic attacks; it does not validate real bus interception, transfer to unseen cells, multi-cell packs, dynamic vehicle loads, total system power, or worst-case task scheduling. A 60-second window is also too slow to serve as the only urgent-hazard response. Future validation should use multiple cells and dynamic loads, measure end-to-end timing while Wi-Fi is active, and assess shorter rolling windows alongside independent safety limits.
