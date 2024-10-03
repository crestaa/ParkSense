#include <WiFi.h>
#include <WiFiAP.h>

#define WIFI_AP_SSID "LowLevelGatewayAP"
#define WIFI_AP_PASS "password123"


WiFiServer server(8080);

void setup() {
  Serial.begin(115200);
  
  // Set up WiFi AP
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String data = client.readStringUntil('\n');
    Serial.println("Received data: " + data);
    
  }
}