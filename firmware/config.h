// ============================================================
// config.h — Pin Definitions & Constants
// Battery Digital Twin - ESP32 Attack Detector
// ============================================================
#ifndef CONFIG_H
#define CONFIG_H

// ─── I2C Pins ───
#define SDA_PIN         21
#define SCL_PIN         22

// ─── Analog Pins ───
#define NTC_ADC_PIN     34    // NTC thermistor

// ─── Control Pins ───
#define MOSFET_PIN      25    // Discharge control

// ─── Sampling Config ───
#define SAMPLE_RATE_HZ       10     // 10 samples/sec
#define WINDOW_SECONDS        60    // 1-minute analysis window
#define WINDOW_SIZE          (SAMPLE_RATE_HZ * WINDOW_SECONDS)  // 600 samples

// ─── NTC Thermistor ───
#define NTC_NOMINAL_R     10000.0f
#define NTC_NOMINAL_T     25.0f
#define NTC_B_COEFFICIENT 3950.0f
#define NTC_SERIES_R      10000.0f

// ─── INA219 ───
#define INA219_I2C_ADDR   0x40

// ─── Attack Classes ───
#define NUM_CLASSES       6
#define NUM_FEATURES      33   // Matches notebook: 10 physics + 20 statistical + 3 extra

// Feature indices (must match the order used during training)
// Physics features (from compute_physics_features)
#define F_PHYSICS_DV_DT_POS_RATIO       0
#define F_PHYSICS_DV_DT_MAX_POS         1
#define F_PHYSICS_TEMP_RATE_VIOLATIONS  2
#define F_PHYSICS_TEMP_RATE_MAX         3
#define F_PHYSICS_V_I_CORRELATION       4
#define F_PHYSICS_V_OUT_OF_BOUNDS       5
#define F_PHYSICS_T_OUT_OF_BOUNDS       6
#define F_PHYSICS_POWER_MEAN            7
#define F_PHYSICS_POWER_STD             8
#define F_PHYSICS_POWER_SPIKES          9
// Statistical features (from extract_statistical_features)
#define F_VOLTAGE_MEAN                  10
#define F_VOLTAGE_STD                   11
#define F_VOLTAGE_MIN                   12
#define F_VOLTAGE_MAX                   13
#define F_VOLTAGE_RANGE                 14
#define F_VOLTAGE_SLOPE                 15
#define F_CURRENT_MEAN                  16
#define F_CURRENT_STD                   17
#define F_CURRENT_ABS_MEAN              18
#define F_TEMP_MEAN                     19
#define F_TEMP_STD                      20
#define F_TEMP_MIN                      21
#define F_TEMP_MAX                      22
#define F_TEMP_RANGE                    23
#define F_TEMP_RISE                     24
#define F_DURATION                      25
#define F_TEMP_RATE_MAX_STAT            26
#define F_DTEMP_DT_STD                  27
#define F_V_AUTOCORR_LAG1               28
#define F_T_AUTOCORR_LAG1               29
// Note: v_jump_max, t_jump_max, energy_discharged are computed
// but may or may not be in the final trained model's feature list.
// The exact ordering MUST match the order from df_attack[feature_cols].
// Run the conversion script to verify the exact feature order.

#endif
