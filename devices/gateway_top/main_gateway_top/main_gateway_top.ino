#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define RF_FREQUENCY 868000000 // Hz
#define LORA_BANDWIDTH 0 // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 12 // [SF7..SF12]
#define LORA_CODINGRATE 4 // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH 8 // SAME FOR TX AND RX
#define LORA_SYMBOL_TIMEOUT 0 
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 3000
#define BUFFER_SIZE 128

// OLED Display
SSD1306Wire disp(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

char rxpacket[BUFFER_SIZE];
static RadioEvents_t RadioEvents;
String clientId; // to store MAC address

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  clientId = WiFi.macAddress();
  clientId.replace(":", "");

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
  Serial.println("MAC address: " + clientId);

  disp.clear();
  disp.drawString(0, 0, "WiFi Connected");
  disp.drawString(0, 15, "IP: " + WiFi.localIP().toString());
  disp.drawString(0, 30, "MAC: " + clientId);
  disp.display();
  delay(2000);
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected with client ID: " + clientId);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void publishSensorData(const char* payload) {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.publish(MQTT_TOPIC, payload);
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  
  Serial.printf("\r\nReceived packet \"%s\" with RSSI %d , SNR %d\r\n", rxpacket, rssi, snr);

  disp.clear();
  disp.drawString(0, 0, "Received Data:");
  disp.drawString(0, 15, String(rxpacket));
  disp.drawString(0, 30, "RSSI: " + String(rssi) + " dBm");
  disp.drawString(0, 45, "SNR: " + String(snr) + " dB");
  disp.display();

  // publish to MQTT broker
  publishSensorData(rxpacket);
}

void setup() {
  Serial.begin(115200);
    disp.init();
  disp.setFont(ArialMT_Plain_10);
  
  // setup WiFi & MQTT
  setupWiFi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  reconnectMQTT();
  
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  
  RadioEvents.RxDone = OnRxDone;
  
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  disp.clear();
  disp.drawString(0, 0, "LoRa Receiver");
  disp.drawString(0, 20, "Listening...");
  disp.display();

  Serial.println("LoRa Receiver initialized. Listening...");
}

void loop() {
  Radio.Rx(0); // continuous receive mode
  delay(100);
  Radio.IrqProcess();
  client.loop();
}