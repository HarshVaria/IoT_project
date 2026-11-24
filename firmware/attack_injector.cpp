// ============================================================
// attack_injector.cpp — Software Attack Injection for Demo
// Mirrors notebook Cell 12 (BatteryAttackSimulator)
// Battery Digital Twin - ESP32 Attack Detector
// ============================================================
#include "attack_injector.h"
#include <math.h>
#include <stdlib.h>
#include <Arduino.h>

const char* AttackInjector::getAttackName(int attackType) {
    switch (attackType) {
        case 0: return "Normal";
        case 1: return "Temp Spike";
        case 2: return "Voltage Offset";
        case 3: return "SOC Drift";
        case 4: return "Random Noise";
        case 5: return "Time Delay";
        default: return "Unknown";
    }
}

void AttackInjector::inject(SensorSample* buffer, int count, int attackType) {
    if (attackType <= 0 || attackType > 5) return;
    
    switch (attackType) {
        case 1: injectTempSpike(buffer, count); break;
        case 2: injectVoltageOffset(buffer, count); break;
        case 3: injectSOCDrift(buffer, count); break;
        case 4: injectRandomNoise(buffer, count); break;
        case 5: injectTimeDelay(buffer, count); break;
    }
}

float AttackInjector::gaussianRandom(float mean, float stddev) {
    // Box-Muller transform
    float u1 = (float)(random(1, 10000)) / 10000.0f;
    float u2 = (float)(random(1, 10000)) / 10000.0f;
    float z0 = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * (float)PI * u2);
    return z0 * stddev + mean;
}

void AttackInjector::injectTempSpike(SensorSample* buffer, int count) {
    float mag = gaussianRandom(37.5f, 5.0f);
    int dur = random(10, 30);
    int st = random(10, max(11, count - dur - 10));
    int ed = min(st + dur, count);
    float center = (st + ed) / 2.0f;
    float width = max(dur / 4.0f, 1.0f);
    for (int i = st; i < ed; i++) {
        buffer[i].temperature += mag * expf(-0.5f * powf((i - center) / width, 2));
    }
}

void AttackInjector::injectVoltageOffset(SensorSample* buffer, int count) {
    float bias = gaussianRandom(0.55f, 0.15f) * (gaussianRandom(0, 1) > 0 ? 1 : -1);
    int st = random(count / 4, count / 2);
    for (int i = st; i < count; i++) {
        buffer[i].voltage += bias;
        if (buffer[i].voltage > 5.0f) buffer[i].voltage = 5.0f;
        if (buffer[i].voltage < 1.5f) buffer[i].voltage = 1.5f;
    }
}

void AttackInjector::injectSOCDrift(SensorSample* buffer, int count) {
    float rate = gaussianRandom(0.0055f, 0.0015f);
    int st = count / 4;
    for (int i = st; i < count; i++) {
        buffer[i].voltage += rate * (i - st);
        if (buffer[i].voltage > 5.0f) buffer[i].voltage = 5.0f;
        if (buffer[i].voltage < 1.5f) buffer[i].voltage = 1.5f;
    }
}

void AttackInjector::injectRandomNoise(SensorSample* buffer, int count) {
    float stddev = gaussianRandom(11.5f, 2.5f);
    int st = random(count / 4, count / 2);
    for (int i = st; i < count; i++) {
        buffer[i].temperature += gaussianRandom(0.0f, stddev);
        if (buffer[i].temperature < -30.0f) buffer[i].temperature = -30.0f;
        if (buffer[i].temperature > 80.0f) buffer[i].temperature = 80.0f;
    }
}

void AttackInjector::injectTimeDelay(SensorSample* buffer, int count) {
    int delay = random(150, 250); // Massive freeze (15-25 seconds) to be detectable
    int st = count / 4;
    if (st + delay < count) {
        for (int i = count - 1; i >= st + delay; i--) {
            buffer[i].voltage = buffer[i - delay].voltage;
            buffer[i].temperature = buffer[i - delay].temperature;
        }
        for (int i = st; i < min(st + delay, count); i++) {
            buffer[i].voltage = buffer[st - 1].voltage;
            buffer[i].temperature = buffer[st - 1].temperature;
        }
    }
}
