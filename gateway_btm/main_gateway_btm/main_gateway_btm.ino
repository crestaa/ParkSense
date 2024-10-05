#include <WiFi.h>
#include <WiFiAP.h>
#include "Wire.h"
#include "HT_SSD1306Wire.h"
#include "config.h"

SSD1306Wire  disp(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

char macStr[18];
WiFiServer server(SERVER_PORT);

void setup() {
  Serial.begin(115200);
  disp.init();
  disp.setFont(ArialMT_Plain_10);
  
  // Get MAC address
  uint8_t mac[6];
  //WiFi.begin();
  WiFi.macAddress(mac);
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  // Create SSID with last 4 digits of MAC address
  char ssid[13];
  snprintf(ssid, sizeof(ssid), "Gateway%02X%02X", mac[4], mac[5]);
  
  // Set up WiFi AP
  WiFi.softAP(ssid, WIFI_PASS, 1, 1);   // hidden = true
  IPAddress myIP = WiFi.softAPIP();
  Serial.println("SSID: "+String(ssid)+", pass: "+String(WIFI_PASS));
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  // Display MAC address and SSID on OLED
  disp.clear();
  disp.drawString(0, 0, "MAC Address:");
  disp.drawString(0, 15, macStr);
  disp.drawString(0, 30, "SSID: " + String(ssid));
  disp.drawString(0, 45, "IP: " + myIP.toString());
  disp.display();
}

void loop() {
  delay(50);

  WiFiClient client = server.available();
  if (client) {
    String data = client.readStringUntil('\n');
    Serial.println("Received data: " + data);
  }
}