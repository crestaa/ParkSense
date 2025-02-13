#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>
#include "config.h"
#include <esp_system.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Wire.h>
#include <VL53L0X.h>

DHT dht(DHTPIN, DHTTYPE);
Preferences preferences;
VL53L0X sensor;

uint8_t gatewayAddress[] = ESPNOW_GATEWAY;

#define uS_TO_S_FACTOR 1000000ULL  // micro seconds to seconds
int bootCount = 0;

esp_now_peer_info_t peerInfo;

// MAC address (last 3 bytes)
char deviceMacStr[7];  // 6 chars + null terminator

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print("Tried sending to: ");
    Serial.println(macStr);
    
    Serial.println("Going to sleep for " + String(TIME_TO_SLEEP) + " seconds");
    Serial.flush();
    esp_deep_sleep_start();
}

void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
    esp_wifi_set_channel(1, secondChan);
    
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    
    snprintf(deviceMacStr, sizeof(deviceMacStr), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    
    Serial.print("Device MAC bytes: ");
    for(int i = 0; i < 6; i++) {
        Serial.printf("%02X:", mac[i]);
    }
    Serial.println();
    
    Serial.print("Target Gateway MAC: ");
    for(int i = 0; i < 6; i++) {
        Serial.printf("%02X:", gatewayAddress[i]);
    }
    Serial.println();
}

float readDistance() {
    const int NUM_READINGS = 4;
    const float MAX_DEVIATION_PERCENT = 15.0;  // 15% maximum deviation from mean
    
    float readings[NUM_READINGS];
    float sum = 0;
    int validReadings = 0;
    
    Serial.println("Taking distance readings...");
    
    for (int i = 0; i < NUM_READINGS; i++) {
        uint16_t distance = sensor.readRangeSingleMillimeters();
        
        if (sensor.timeoutOccurred()) {
            Serial.println("Timeout on reading " + String(i + 1));
            readings[i] = -1;
            continue;
        }
        
        float distanceCm = distance / 10.0;

        Serial.println("Reading " + String(i + 1) + ": " + String(distanceCm) + "cm");
        readings[i] = distanceCm;
        sum += distanceCm;
        validReadings++;
        
        delay(50); 
    }
    
    if (validReadings < 2) {
        return -1;
    }
    
    // initial mean
    float mean = sum / validReadings;
    
    sum = 0;
    int finalValidReadings = 0;
    
    for (int i = 0; i < NUM_READINGS; i++) {
        if (readings[i] == -1) continue;
        
        float deviation = abs(readings[i] - mean) / mean * 100;
        
        // if reading is within acceptable deviation, include it
        if (deviation <= MAX_DEVIATION_PERCENT) {
            sum += readings[i];
            finalValidReadings++;
        }
    }
    
    if (finalValidReadings < 2) {
        return -1;
    }
    
    return sum / finalValidReadings;
}

void performTask() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float distance = readDistance();
    
    // JSON document
    StaticJsonDocument<200> doc;
    bool hasValidData = false;
    
    if (!isnan(h) && !isnan(t)) {
        doc["t"] = t;
        doc["h"] = h;
        hasValidData = true;
    } else {
        Serial.println("Failed to read from DHT sensor");
    }
    
    if (distance != -1) {
        doc["d"] = distance;
        hasValidData = true;
    } else {
        Serial.println("Failed to read from VL53L0X sensor");
    }
    
    if (hasValidData) {
        doc["m"] = deviceMacStr;
        doc["b"] = bootCount;
        
        // Serialize JSON to string
        String jsonString;
        serializeJson(doc, jsonString);
        
        Serial.println("Sending data: " + jsonString);
        
        // send data using ESP-NOW
        esp_err_t result = esp_now_send(gatewayAddress, (uint8_t *)jsonString.c_str(), jsonString.length() + 1);
        
        if (result == ESP_OK) {
            Serial.println("Sent with success");
        } else {
            Serial.println("Error sending the data");
            // if sending fails, go to sleep immediately
            preferences.end();  // close preferences before sleep
            esp_deep_sleep_start();
        }
    } else {
        Serial.println("No valid data to send");
        preferences.end();  // close preferences before sleep
        esp_deep_sleep_start();
    }
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    
    // initialize I2C and VL53L0X
    Wire.begin(SDA_PIN, SCL_PIN); 
    sensor.init();
    sensor.setTimeout(500);  // Set timeout to 500ms
    
    sensor.setMeasurementTimingBudget(200000);  // Set timing budget to 200ms
    
    preferences.begin("sensor", false);
    bootCount = preferences.getInt("bootCount", 0);
    bootCount++;
    preferences.putInt("bootCount", bootCount);
    Serial.println("Boot number: " + String(bootCount));
    
    initWiFi();
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    memcpy(peerInfo.peer_addr, gatewayAddress, 6);
    peerInfo.channel = 1;  // MUST MATCH GATEWAY CHANNEL
    peerInfo.encrypt = false;      // possible future work: implement encryption
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
    
    // callback
    esp_now_register_send_cb(OnDataSent);
    
    // deep sleep
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    
    delay(1000);
    performTask();
}

void loop() {
    // not used when using deep sleep
}