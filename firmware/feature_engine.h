// ============================================================
// feature_engine.h — Feature Engineering (matches notebook Cell 15)
// Battery Digital Twin - ESP32 Attack Detector
// ============================================================
#ifndef FEATURE_ENGINE_H
#define FEATURE_ENGINE_H

#include "sensor_reader.h"
#include "config.h"

class FeatureEngine {
public:
    // Compute all features from a window of samples.
    // Returns the number of features written to `output`.
    // `output` must be at least NUM_FEATURES floats.
    int compute(SensorSample* samples, int count, float* output);

private:
    // ─── Statistical helpers ───
    float calcMean(float* arr, int n);
    float calcStd(float* arr, int n, float mean_val);
    float calcMin(float* arr, int n);
    float calcMax(float* arr, int n);
    float calcPtp(float* arr, int n);  // peak-to-peak (range)
    float calcCorrcoef(float* a, float* b, int n);
    float calcMaxAbsDiff(float* arr, int n);  // max |diff|
    float calcTrapz(float* y, float* x, int n);  // trapezoidal integration
};

#endif
