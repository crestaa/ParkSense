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
    delay(500);
    String data = "temp:" + String(t) + ",hum:" + String(h);
    client.println(data);
    client.stop();
    Serial.println("Data sent: " + data);
  } else {
    if(WiFi.status() != WL_CONNECTED){
      while (WiFi.status() != WL_CONNECTED) {
        delay(2000);
        Serial.println("Connecting to WiFi "+ String(ssid) + " with password "+ String(password) + " RSSI: "+String(WiFi.RSSI()));
      }
    }else{
        Serial.println("Couldn't connect to server");
    }
    
  }

  delay(30000);  // Wait for 30 seconds before next reading
}