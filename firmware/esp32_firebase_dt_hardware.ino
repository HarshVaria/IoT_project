
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#include "config.h"
#include "sensor_reader.h"
#include "feature_engine.h"
#include "attack_injector.h"
#include "attack_model_wrapper.h"  // ← Uses the new wrapper to massively speed up compilation

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

#define API_KEY "YOUR_FIREBASE_API_KEY" // Leaked key removed. Insert your regenerated API key here.
#define DATABASE_URL "https://bms-simulation-web-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig fbConfig;

SensorReader sensors;
FeatureEngine featureEngine;
AttackInjector attackInjector;

SensorSample sampleBuffer[WINDOW_SIZE];
int sampleIndex = 0;
unsigned long lastSampleTime = 0;
unsigned long lastCloudSync = 0;

volatile bool monitoring = false;
volatile bool discharging = false;
volatile int pendingAttack = -1;
volatile bool newResultAvailable = false;
TaskHandle_t FirebaseTask;

struct DetectionResult {
    int predicted;
    float features[NUM_FEATURES + 5];
    int numFeatures;
    unsigned long timestamp;
    bool valid;
} lastResult;
SensorSample latestSample;

void firebaseTaskCode(void * parameter) {
    unsigned long lastCloudSync = 0;
    for (;;) {
        // 1. Sync every 500ms
        if (millis() - lastCloudSync >= 500) {
            lastCloudSync = millis();
            
            if (Firebase.RTDB.getBool(&fbdo, "/esp32/commands/monitoring")) {
                bool cloudMonitoring = fbdo.boolData();
                if (cloudMonitoring != monitoring) {
                    monitoring = cloudMonitoring;
                    if (monitoring) sampleIndex = 0; // reset
                }
            }
            
            if (Firebase.RTDB.getBool(&fbdo, "/esp32/commands/discharging")) {
                discharging = fbdo.boolData();
                digitalWrite(MOSFET_PIN, discharging ? HIGH : LOW);
            }

            if (Firebase.RTDB.getInt(&fbdo, "/esp32/commands/injectAttack")) {
                int attackId = fbdo.intData();
                if (attackId >= 0 && attackId <= 5) {
                    pendingAttack = attackId;
                    Firebase.RTDB.setInt(&fbdo, "/esp32/commands/injectAttack", -1);
                }
            }

            // Push telemetry back to the cloud
            FirebaseJson json;
            json.set("voltage", latestSample.voltage);
            json.set("current", latestSample.current);
            json.set("temperature", latestSample.temperature);
            json.set("sampleIndex", sampleIndex);
            json.set("windowSize", WINDOW_SIZE);
            json.set("monitoring", monitoring);
            json.set("discharging", discharging);
            json.set("resultValid", lastResult.valid);
            json.set("predicted", lastResult.predicted);
            json.set("numFeatures", lastResult.numFeatures);
            
            Firebase.RTDB.updateNode(&fbdo, "/esp32/status", &json);
        }

        // 2. Push Result to Firebase if new result is available
        if (newResultAvailable) {
            FirebaseJson resultJson;
            resultJson.set("predicted", lastResult.predicted);
            resultJson.set("valid", true);
            resultJson.set("numFeatures", lastResult.numFeatures);
            
            FirebaseJsonArray featuresArray;
            for(int i = 0; i < lastResult.numFeatures; i++) {
                featuresArray.add((double)lastResult.features[i]);
            }
            resultJson.set("features", featuresArray);

            Firebase.RTDB.setJSON(&fbdo, "/esp32/result", &resultJson);
            newResultAvailable = false;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // Yield to watchdog
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, LOW);
    sensors.begin();
    lastResult.valid = false;
    lastResult.predicted = -1;

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());

    fbConfig.api_key = API_KEY;
    fbConfig.database_url = DATABASE_URL;
    
    // Sign up anonymously or simply use API Key if rules are open
    if (Firebase.signUp(&fbConfig, &auth, "", "")) {
        Serial.println("Firebase Auth successful");
    } else {
        Serial.printf("Firebase Auth Error: %s\n", fbConfig.signer.signupError.message.c_str());
    }

    fbConfig.token_status_callback = tokenStatusCallback;
    Firebase.begin(&fbConfig, &auth);
    Firebase.reconnectWiFi(true);

    // Start Firebase Task on Core 0
    xTaskCreatePinnedToCore(
        firebaseTaskCode, /* Task function. */
        "FirebaseTask",   /* name of task. */
        10000,            /* Stack size of task */
        NULL,             /* parameter of the task */
        1,                /* priority of the task */
        &FirebaseTask,    /* Task handle to keep track of created task */
        0);               /* pin task to core 0 */
}

void loop() {
    if (monitoring) {
        unsigned long now = millis();
        if (now - lastSampleTime >= (1000 / SAMPLE_RATE_HZ)) {
            lastSampleTime = now;
            latestSample = sensors.read();
            sampleBuffer[sampleIndex] = latestSample;
            sampleIndex++;

            if (sampleIndex >= WINDOW_SIZE) {
                runDetectionPipeline();
                sampleIndex = 0;
            }
        }
    }
}

void runDetectionPipeline() {
    if (pendingAttack >= 0) {
        attackInjector.inject(sampleBuffer, WINDOW_SIZE, pendingAttack);
        pendingAttack = -1;
    }

    float features[NUM_FEATURES + 5];
    int nFeatures = featureEngine.compute(sampleBuffer, WINDOW_SIZE, features);
    int predicted = attack_model_predict_wrapper(features, nFeatures);

    lastResult.predicted = predicted;
    lastResult.numFeatures = nFeatures;
    lastResult.timestamp = millis();
    lastResult.valid = true;
    
    // Save computed features into lastResult for the cloud update if needed
    for (int i = 0; i < nFeatures; i++) {
        lastResult.features[i] = features[i];
    }

    newResultAvailable = true; // Signal Core 0 to push the result
}

