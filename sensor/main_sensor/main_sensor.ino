#include <WiFi.h>
#include "config.h"
#include <DHT.h>

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const char* serverIP = SERVER_IP; 
const int serverPort = SERVER_PORT;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

String macAddress;

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds

RTC_DATA_ATTR int bootCount = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  // HC-SR04 setup
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // WiFi connection setup
  randomSeed(micros());
  int randomByte =  random(20, 201);
  Serial.println(randomByte);
  IPAddress local_IP(192, 168, 4, randomByte);
  IPAddress gateway(192, 168, 4, 1);    
  IPAddress subnet(255, 255, 255, 0);   
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.println("Connecting to WiFi "+ String(ssid) + " with password "+ String(password) + " RSSI: "+String(WiFi.RSSI()));
  }
  Serial.println("Connected to WiFi");

  // Get the last 6 characters of MAC address
  macAddress = WiFi.macAddress();
  macAddress.replace(":", "");  // Remove colons
  macAddress = macAddress.substring(macAddress.length() - 6);  // Get last 6 characters

  delay(1000);
  performTask();

  // Prepare for deep sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Going to sleep for " + String(TIME_TO_SLEEP) + " seconds");
  Serial.flush(); 
  esp_deep_sleep_start();
}

float readDistance() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;  // Speed of sound wave divided by 2 (go and back)
  
  if (distance > 400 || distance <= 2) {  // HC-SR04 typical range is 2cm to 400cm
    return -1;  // Invalid reading
  }
  return distance;
}

void performTask() {
  float h = dht.readHumidity();      
  float t = dht.readTemperature();
  float distance = readDistance();

  String data = "";
  bool hasValidData = false;

  if (!isnan(h) && !isnan(t)) {
    data += "t:" + String(t) + ",h:" + String(h);
    hasValidData = true;
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  if (distance != -1) {
    if (hasValidData) data += ",";
    data += "d:" + String(distance);
    hasValidData = true;
  } else {
    Serial.println("Failed to read from HC-SR04 sensor!");
  }

  // Generate random nonce
  long nonce = random(1000, 100001);  // Random number between 1000 and 100000

  // Append MAC address and nonce to data
  if (hasValidData) {
    data += ",m:" + macAddress + ",n:" + String(nonce);
  }

  if (hasValidData && client.connect(serverIP, serverPort)) {
    delay(500);
    client.println(data);
    client.stop();
    Serial.println("Data sent: " + data);
  } else {
    if (WiFi.status() != WL_CONNECTED) {
      while (WiFi.status() != WL_CONNECTED) {
        delay(2000);
        Serial.println("Connecting to WiFi "+ String(ssid) + " with password "+ String(password) + " RSSI: "+String(WiFi.RSSI()));
      }
    } else if (!hasValidData) {
      Serial.println("No valid data to send");
    } else {
      Serial.println("Couldn't connect to server");
    }
  }
}

void loop() {
  // This function is not used when using deep sleep
}