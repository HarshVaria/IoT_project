// ============================================================
// feature_engine.cpp — Feature Engineering Implementation
// Faithfully mirrors the Python notebook (Cell 15):
//   compute_physics_features() + extract_statistical_features()
// Battery Digital Twin - ESP32 Attack Detector
// ============================================================
#include "feature_engine.h"
#include <math.h>
#include <string.h>

// ─── Statistical helpers ─────────────────────────────────────

float FeatureEngine::calcMean(float* arr, int n) {
    float sum = 0;
    for (int i = 0; i < n; i++) sum += arr[i];
    return sum / (float)n;
}

float FeatureEngine::calcStd(float* arr, int n, float mean_val) {
    float sum_sq = 0;
    for (int i = 0; i < n; i++) {
        float diff = arr[i] - mean_val;
        sum_sq += diff * diff;
    }
    return sqrtf(sum_sq / (float)n);
}

float FeatureEngine::calcMin(float* arr, int n) {
    float mn = arr[0];
    for (int i = 1; i < n; i++) if (arr[i] < mn) mn = arr[i];
    return mn;
}

float FeatureEngine::calcMax(float* arr, int n) {
    float mx = arr[0];
    for (int i = 1; i < n; i++) if (arr[i] > mx) mx = arr[i];
    return mx;
}

float FeatureEngine::calcPtp(float* arr, int n) {
    return calcMax(arr, n) - calcMin(arr, n);
}

float FeatureEngine::calcCorrcoef(float* a, float* b, int n) {
    float ma = calcMean(a, n);
    float mb = calcMean(b, n);
    float num = 0, da = 0, db = 0;
    for (int i = 0; i < n; i++) {
        float da_i = a[i] - ma;
        float db_i = b[i] - mb;
        num += da_i * db_i;
        da  += da_i * da_i;
        db  += db_i * db_i;
    }
    float denom = sqrtf(da * db);
    return (denom > 1e-9f) ? num / denom : 0.0f;
}

float FeatureEngine::calcMaxAbsDiff(float* arr, int n) {
    float mx = 0;
    for (int i = 1; i < n; i++) {
        float d = fabsf(arr[i] - arr[i-1]);
        if (d > mx) mx = d;
    }
    return mx;
}

float FeatureEngine::calcTrapz(float* y, float* x, int n) {
    // Trapezoidal integration (mirrors np.trapz)
    float total = 0;
    for (int i = 1; i < n; i++) {
        float dx = x[i] - x[i-1];
        total += 0.5f * (y[i] + y[i-1]) * dx;
    }
    return total;
}


// ─── Main feature computation ────────────────────────────────
// This mirrors Cell 15 of the notebook EXACTLY.
// Feature order: physics features first, then statistical features,
// exactly as produced by extract_all_features() = {**physics, **stats}

int FeatureEngine::compute(SensorSample* samples, int count, float* output) {
    if (count < 5) return 0;

    // Extract raw arrays from SensorSample buffer
    // Using static to avoid stack overflow with large windows
    static float v[WINDOW_SIZE];
    static float curr[WINDOW_SIZE];
    static float temp[WINDOW_SIZE];
    static float tv[WINDOW_SIZE];

    for (int i = 0; i < count; i++) {
        v[i]    = samples[i].voltage;
        curr[i] = samples[i].current;
        temp[i] = samples[i].temperature;
        tv[i]   = samples[i].timestamp_sec;
    }

    int fi = 0;  // feature index

    // ═══════════════════════════════════════════════════════════
    // PHYSICS FEATURES (compute_physics_features from notebook)
    // ═══════════════════════════════════════════════════════════

    // dt = np.diff(tv); dt[dt <= 0] = 1e-6
    static float dt[WINDOW_SIZE];
    static float dv_dt[WINDOW_SIZE];
    static float dt_dt[WINDOW_SIZE];
    int ndiff = count - 1;

    for (int i = 0; i < ndiff; i++) {
        dt[i] = tv[i+1] - tv[i];
        if (dt[i] <= 0) dt[i] = 1e-6f;
    }

    // dv_dt = np.diff(v) / dt
    for (int i = 0; i < ndiff; i++) {
        dv_dt[i] = (v[i+1] - v[i]) / dt[i];
    }

    // physics_dv_dt_positive_ratio = np.mean(dv_dt > 0)
    int pos_count = 0;
    for (int i = 0; i < ndiff; i++) {
        if (dv_dt[i] > 0) pos_count++;
    }
    output[fi++] = (float)pos_count / (float)ndiff;

    // physics_dv_dt_max_positive = np.max(dv_dt)
    float max_dv = dv_dt[0];
    for (int i = 1; i < ndiff; i++) {
        if (dv_dt[i] > max_dv) max_dv = dv_dt[i];
    }
    output[fi++] = max_dv;

    // dt_dt = np.diff(temp) / dt
    for (int i = 0; i < ndiff; i++) {
        dt_dt[i] = (temp[i+1] - temp[i]) / dt[i];
    }

    // physics_temp_rate_violations = np.sum(np.abs(dt_dt) > 0.5) / len(dt_dt)
    int violations = 0;
    for (int i = 0; i < ndiff; i++) {
        if (fabsf(dt_dt[i]) > 0.5f) violations++;
    }
    output[fi++] = (float)violations / (float)ndiff;

    // physics_temp_rate_max = np.max(np.abs(dt_dt))
    float max_dt = fabsf(dt_dt[0]);
    for (int i = 1; i < ndiff; i++) {
        float a = fabsf(dt_dt[i]);
        if (a > max_dt) max_dt = a;
    }
    output[fi++] = max_dt;

    // physics_v_i_correlation
    float curr_std = calcStd(curr, count, calcMean(curr, count));
    if (curr_std > 0.01f) {
        // np.corrcoef(v[1:], np.abs(curr[1:]))[0,1]
        static float v_slice[WINDOW_SIZE];
        static float curr_abs_slice[WINDOW_SIZE];
        for (int i = 1; i < count; i++) {
            v_slice[i-1] = v[i];
            curr_abs_slice[i-1] = fabsf(curr[i]);
        }
        output[fi++] = calcCorrcoef(v_slice, curr_abs_slice, count - 1);
    } else {
        output[fi++] = 0.0f;
    }

    // physics_v_out_of_bounds = np.mean((v < 2.0) | (v > 4.5))
    int v_oob = 0;
    for (int i = 0; i < count; i++) {
        if (v[i] < 2.0f || v[i] > 4.5f) v_oob++;
    }
    output[fi++] = (float)v_oob / (float)count;

    // physics_t_out_of_bounds = np.mean((temp < 0) | (temp > 60))
    int t_oob = 0;
    for (int i = 0; i < count; i++) {
        if (temp[i] < 0.0f || temp[i] > 60.0f) t_oob++;
    }
    output[fi++] = (float)t_oob / (float)count;

    // Power features
    static float power[WINDOW_SIZE];
    for (int i = 0; i < count; i++) {
        power[i] = fabsf(v[i] * curr[i]);
    }

    float power_mean = calcMean(power, count);
    float power_std  = calcStd(power, count, power_mean);
    output[fi++] = power_mean;  // physics_power_mean
    output[fi++] = power_std;   // physics_power_std

    // physics_power_spikes = np.sum(power > np.percentile(power, 99) * 1.5) / len(power)
    // Approximate P99: sort power, take index at 99%
    // Using a simple approach: find the 99th percentile via partial sort
    static float power_sorted[WINDOW_SIZE];
    memcpy(power_sorted, power, count * sizeof(float));
    // Simple selection for P99 (index = count * 0.99)
    int p99_idx = (int)(count * 0.99f);
    if (p99_idx >= count) p99_idx = count - 1;
    // Partial sort up to p99_idx (simple bubble for small N)
    for (int i = 0; i <= p99_idx; i++) {
        for (int j = i + 1; j < count; j++) {
            if (power_sorted[j] < power_sorted[i]) {
                float tmp = power_sorted[i];
                power_sorted[i] = power_sorted[j];
                power_sorted[j] = tmp;
            }
        }
    }
    float p99_val = power_sorted[p99_idx];
    float spike_threshold = p99_val * 1.5f;
    int spike_count = 0;
    for (int i = 0; i < count; i++) {
        if (power[i] > spike_threshold) spike_count++;
    }
    output[fi++] = (float)spike_count / (float)count;  // physics_power_spikes


    // ═══════════════════════════════════════════════════════════
    // STATISTICAL FEATURES (extract_statistical_features from notebook)
    // ═══════════════════════════════════════════════════════════

    float v_mean  = calcMean(v, count);
    float v_std   = calcStd(v, count, v_mean);
    float t_mean  = calcMean(temp, count);
    float t_std   = calcStd(temp, count, t_mean);
    float c_mean  = calcMean(curr, count);
    float c_std   = calcStd(curr, count, c_mean);

    output[fi++] = v_mean;                              // voltage_mean
    output[fi++] = v_std;                               // voltage_std
    output[fi++] = calcMin(v, count);                   // voltage_min
    output[fi++] = calcMax(v, count);                   // voltage_max
    output[fi++] = calcPtp(v, count);                   // voltage_range
    // voltage_slope = (v[-1] - v[0]) / max(tv[-1] - tv[0], 1e-6)
    float duration = tv[count-1] - tv[0];
    if (duration < 1e-6f) duration = 1e-6f;
    output[fi++] = (v[count-1] - v[0]) / duration;     // voltage_slope

    output[fi++] = c_mean;                              // current_mean
    output[fi++] = c_std;                               // current_std
    // current_abs_mean
    float abs_sum = 0;
    for (int i = 0; i < count; i++) abs_sum += fabsf(curr[i]);
    output[fi++] = abs_sum / (float)count;              // current_abs_mean

    output[fi++] = t_mean;                              // temp_mean
    output[fi++] = t_std;                               // temp_std
    output[fi++] = calcMin(temp, count);                // temp_min
    output[fi++] = calcMax(temp, count);                // temp_max
    output[fi++] = calcPtp(temp, count);                // temp_range
    output[fi++] = temp[count-1] - temp[0];             // temp_rise
    output[fi++] = duration;                            // duration

    // temp_rate_max (statistical version, same as physics but from diff perspective)
    // dtemp = np.diff(temp) / dt
    // temp_rate_max = np.max(np.abs(dtemp))
    static float dtemp[WINDOW_SIZE];
    for (int i = 0; i < ndiff; i++) {
        dtemp[i] = (temp[i+1] - temp[i]) / dt[i];
    }
    float stat_temp_rate_max = fabsf(dtemp[0]);
    for (int i = 1; i < ndiff; i++) {
        float a = fabsf(dtemp[i]);
        if (a > stat_temp_rate_max) stat_temp_rate_max = a;
    }
    output[fi++] = stat_temp_rate_max;                  // temp_rate_max

    // dtemp_dt_std = np.std(dtemp)
    float dtemp_mean = calcMean(dtemp, ndiff);
    output[fi++] = calcStd(dtemp, ndiff, dtemp_mean);   // dtemp_dt_std

    // v_autocorr_lag1 = np.corrcoef(v[:-1], v[1:])[0,1]
    output[fi++] = calcCorrcoef(v, v + 1, count - 1);   // v_autocorr_lag1

    // t_autocorr_lag1 = np.corrcoef(temp[:-1], temp[1:])[0,1]
    output[fi++] = calcCorrcoef(temp, temp + 1, count - 1); // t_autocorr_lag1

    // v_jump_max, t_jump_max, energy_discharged
    output[fi++] = calcMaxAbsDiff(v, count);       // v_jump_max
    output[fi++] = calcMaxAbsDiff(temp, count);     // t_jump_max
    float energy = calcTrapz(power, tv, count) / 3600.0f;
    output[fi++] = energy;                          // energy_discharged

    return fi;  // Return number of features written
}
