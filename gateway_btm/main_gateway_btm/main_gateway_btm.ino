#include <WiFi.h>
#include <heltec.h>
// TODO : implement meshtatic / LoRa messages

const char *ssid = "Heltec_AP";
const char *password = "12345678"; // Minimum 8 characters for WPA2

WiFiServer server(80);

void setup() {
  Serial.begin(9600);

  // Initialize Heltec board and display
  Heltec.begin(true, true, true, true, 868000000);

  // Start the WiFi in Access Point mode
  WiFi.softAP(ssid, password);

  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Initialize Meshtastic
  //Meshtastic.begin();

  server.begin(); // Start the server
}

void loop() {
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {
    Serial.println("New client connected");
    String incomingData = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        incomingData += c;

        if (c == '\n') {
          Serial.println("Received data: " + incomingData);
          client.stop();  // Close the connection after receiving data
        }
      }
    }
  }
}
