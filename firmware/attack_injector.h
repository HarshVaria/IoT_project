// ============================================================
// attack_injector.h — Software Attack Injection for Demo
// Mirrors notebook Cell 12 (BatteryAttackSimulator)
// Battery Digital Twin - ESP32 Attack Detector
// ============================================================
#ifndef ATTACK_INJECTOR_H
#define ATTACK_INJECTOR_H

#include "sensor_reader.h"
#include "config.h"

class AttackInjector {
public:
    // Inject an attack into the sample buffer (modifies in-place)
    // attackType: 0=Normal, 1=TempSpike, 2=VoltOffset, 3=SOCDrift,
    //             4=RandomNoise, 5=TimeDelay
    void inject(SensorSample* buffer, int count, int attackType);

    // Get attack name string
    static const char* getAttackName(int attackType);

private:
    void injectTempSpike(SensorSample* buffer, int count);
    void injectVoltageOffset(SensorSample* buffer, int count);
    void injectSOCDrift(SensorSample* buffer, int count);
    void injectRandomNoise(SensorSample* buffer, int count);
    void injectTimeDelay(SensorSample* buffer, int count);

    // Simple Box-Muller gaussian random
    float gaussianRandom(float mean, float stddev);
};

#endif
