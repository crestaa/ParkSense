#include <WiFi.h>
#include "config.h"
//#include <DHT.h>

//#define DHTPIN 4     // Digital pin connected to the DHT sensor
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const char* serverIP = SERVER_IP;  // IP of the Low Level Gateway
const int serverPort = SERVER_PORT;

//DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

void setup() {
  Serial.begin(115200);
  //dht.begin();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi "+ String(ssid) + " with password "+ String(password));
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  // TODO: implement sensor reads, remove mockups 
  float h = 60.7;//dht.readHumidity();      
  float t = 32.2;//dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  if (client.connect(serverIP, serverPort)) {
    // TODO: improve interface for sending data
    String data = "temp:" + String(t) + ",hum:" + String(h);
    client.println(data);
    client.stop();
    Serial.println("Data sent: " + data);
  } else {
    Serial.println("Couldn't connect to server");
  }

  delay(30000);  // Wait for 30 seconds before next reading
}