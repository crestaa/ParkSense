#include <WiFi.h>
//#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT22

const char* ssid = "Heltec_AP";
const char* password = "12345678"; // Same as the one set in AP

const char* host = "192.168.4.1";  // Default IP of the Heltec AP

//DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  //dht.begin();

  // Connect to the Heltec Access Point
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

void loop() {
  WiFiClient client;
  
  if (!client.connect(host, 80)) {
    Serial.println("Connection to Heltec AP failed");
    delay(5000);  // Retry after 5 seconds
    return;
  }

  // Read DHT22 sensor data
  //float humidity = dht.readHumidity();
  //float temperature = dht.readTemperature();
  float humidity = 0.1;
  float temperature = 0.2;

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Send data to Heltec AP
  String data = String(temperature) + "," + String(humidity);
  client.println(data);
  Serial.println("Data sent: " + data);

  client.stop();  // Close the connection
  delay(2000);    // Wait before sending the next data
}
