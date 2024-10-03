#include <LoRaWan_APP.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"

#define MQTT_SERVER "your_mqtt_broker_ip"
#define MQTT_PORT 1883
#define MQTT_USER "your_mqtt_username"
#define MQTT_PASSWORD "your_mqtt_password"

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Connect to MQTT broker
  client.setServer(MQTT_SERVER, MQTT_PORT);

  // Initialize LoRaWAN
  LoRaWAN.begin(LORAWAN_CLASS, ACTIVE_REGION);
  LoRaWAN.joinABP("DevAddr", "NwkSKey", "AppSKey");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Check for incoming LoRaWAN messages
  if (LoRaWAN.parsePacket()) {
    String data = "";
    while (LoRaWAN.available()) {
      data += (char)LoRaWAN.read();
    }
    Serial.println("Received LoRaWAN data: " + data);

    // Parse and publish data to MQTT
    int separatorIndex = data.indexOf(',');
    if (separatorIndex != -1) {
      String temp = data.substring(5, separatorIndex);
      String hum = data.substring(separatorIndex + 5);
      
      client.publish("/temp", temp.c_str());
      client.publish("/hum", hum.c_str());
      
      Serial.println("Published to MQTT - Temp: " + temp + ", Hum: " + hum);
    }
  }

  LoRaWAN.loop();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("HeltecGateway", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}