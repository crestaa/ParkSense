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

// Replace with the Primary MAC shown on your gateway's OLED and Serial output
uint8_t gatewayAddress[] = ESPNOW_GATEWAY;

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
int bootCount = 0;

// ESP-NOW peer information
esp_now_peer_info_t peerInfo;

// Store MAC address
char deviceMacStr[7];  // 6 chars + null terminator

// Callback when data is sent
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
    // Initialize WiFi in station mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    // Configure WiFi channel
    wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
    esp_wifi_set_channel(1, secondChan);
    
    // Get the ESP32's MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    
    // Format the last 6 digits of MAC address
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
    
    // Take 4 readings
    for (int i = 0; i < NUM_READINGS; i++) {
        uint16_t distance = sensor.readRangeSingleMillimeters();
        
        if (sensor.timeoutOccurred()) {
            Serial.println("Timeout on reading " + String(i + 1));
            readings[i] = -1;
            continue;
        }
        
        // Convert to cm and check if timeout occurred
        float distanceCm = distance / 10.0;
        
        // Check for timeout or out of range readings
        // VL53L0X typical range is 3cm to 120cm
        if (distanceCm > 120 || distanceCm < 3) {
            Serial.println("Reading " + String(i + 1) + " out of range: " + String(distanceCm) + "cm");
            readings[i] = -1;
            continue;
        }
        
        Serial.println("Reading " + String(i + 1) + ": " + String(distanceCm) + "cm");
        readings[i] = distanceCm;
        sum += distanceCm;
        validReadings++;
        
        delay(50);  // Short delay between readings
    }
    
    // If we don't have at least 2 valid readings, return error
    if (validReadings < 2) {
        return -1;
    }
    
    // Calculate initial mean
    float mean = sum / validReadings;
    
    // Remove outliers and recalculate
    sum = 0;
    int finalValidReadings = 0;
    
    for (int i = 0; i < NUM_READINGS; i++) {
        if (readings[i] == -1) continue;
        
        // Calculate deviation percentage from mean
        float deviation = abs(readings[i] - mean) / mean * 100;
        
        // If reading is within acceptable deviation, include it
        if (deviation <= MAX_DEVIATION_PERCENT) {
            sum += readings[i];
            finalValidReadings++;
        }
    }
    
    // If we don't have at least 2 readings after filtering, return error
    if (finalValidReadings < 2) {
        return -1;
    }
    
    // Return final filtered average
    return sum / finalValidReadings;
}

void performTask() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float distance = readDistance();
    
    // Create JSON document
    StaticJsonDocument<200> doc;
    bool hasValidData = false;
    
    if (!isnan(h) && !isnan(t)) {
        doc["t"] = t;
        doc["h"] = h;
        hasValidData = true;
    } else {
        Serial.println("Failed to read from DHT sensor!");
    }
    
    if (distance != -1) {
        doc["d"] = distance;
        hasValidData = true;
    } else {
        Serial.println("Failed to read from VL53L0X sensor!");
    }
    
    // Add metadata
    if (hasValidData) {
        doc["m"] = deviceMacStr;
        doc["b"] = bootCount;
        
        // Serialize JSON to string
        String jsonString;
        serializeJson(doc, jsonString);
        
        Serial.println("Sending data: " + jsonString);
        
        // Send data using ESP-NOW
        esp_err_t result = esp_now_send(gatewayAddress, (uint8_t *)jsonString.c_str(), jsonString.length() + 1);
        
        if (result == ESP_OK) {
            Serial.println("Sent with success");
        } else {
            Serial.println("Error sending the data");
            // If sending fails, go to sleep immediately
            preferences.end();  // Close preferences before sleep
            esp_deep_sleep_start();
        }
    } else {
        Serial.println("No valid data to send");
        preferences.end();  // Close preferences before sleep
        esp_deep_sleep_start();
    }
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    
    // Initialize I2C and VL53L0X
    Wire.begin(SDA_PIN, SCL_PIN); 
    sensor.init();
    sensor.setTimeout(500);  // Set timeout to 500ms
    
    // Optional: Configure VL53L0X for better accuracy
    sensor.setMeasurementTimingBudget(200000);  // Set timing budget to 200ms
    
    // Initialize Preferences
    preferences.begin("sensor", false);
    
    // Get stored boot count, default to 0 if not found
    bootCount = preferences.getInt("bootCount", 0);
    bootCount++;
    preferences.putInt("bootCount", bootCount);
    Serial.println("Boot number: " + String(bootCount));
    
    // Initialize WiFi and get MAC
    initWiFi();
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    // Register peer
    memcpy(peerInfo.peer_addr, gatewayAddress, 6);
    peerInfo.channel = 1;  // Must match gateway channel
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
    
    // Register callback
    esp_now_register_send_cb(OnDataSent);
    
    // Prepare for deep sleep
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    
    delay(1000);
    performTask();
}

void loop() {
    // This function is not used when using deep sleep
}