#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>
#include "config.h"
#include <esp_system.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>

DHT dht(DHTPIN, DHTTYPE);

// Replace with the Primary MAC shown on your gateway's OLED and Serial output
uint8_t gatewayAddress[] = ESPNOW_GATEWAY;  // Your gateway MAC

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
RTC_DATA_ATTR int bootCount = 0;

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
        
    // Print gateway MAC we're trying to reach
    Serial.print("Target Gateway MAC: ");
    for(int i = 0; i < 6; i++) {
        Serial.printf("%02X:", gatewayAddress[i]);
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
    
    // HC-SR04 setup
    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
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

float readDistance() {
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034 / 2;
    
    if (distance > 400 || distance <= 2) {
        return -1;
    }
    return distance;
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
        Serial.println("Failed to read from HC-SR04 sensor!");
    }
    
    // Add metadata
    if (hasValidData) {
        doc["m"] = deviceMacStr;
        doc["n"] = random(1000, 100001);
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
            esp_deep_sleep_start();
        }
    } else {
        Serial.println("No valid data to send");
        esp_deep_sleep_start();
    }
}

void loop() {
    // This function is not used when using deep sleep
}