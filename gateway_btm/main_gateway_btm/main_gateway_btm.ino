#include <WiFi.h>
#include <WiFiAP.h>
#include "Wire.h"
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "config.h"

// OLED Display
SSD1306Wire disp(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// WiFi
char macStr[18];
WiFiServer server(SERVER_PORT);

// LoRa
#define RF_FREQUENCY                                868000000 // Hz
#define TX_OUTPUT_POWER                             19        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       12         // [SF7..SF12]
#define LORA_CODINGRATE                             4         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define RX_TIMEOUT_VALUE                            3000
#define BUFFER_SIZE                                 128     // Payload size


char txpacket[BUFFER_SIZE];
bool lora_idle = true;

static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);

void setup() {
  Serial.begin(115200);
  
  // Initialize OLED
  disp.init();
  disp.setFont(ArialMT_Plain_10);
  
  // Get MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  // Create SSID with last 4 digits of MAC address
  char ssid[13];
  snprintf(ssid, sizeof(ssid), "Gateway%02X%02X", mac[4], mac[5]);
  
  // Set up WiFi AP
  WiFi.softAP(ssid, WIFI_PASS, 1, 1, 8); // hidden = true
  IPAddress myIP = WiFi.softAPIP();
  Serial.println("SSID: " + String(ssid) + ", pass: " + String(WIFI_PASS));
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  delay(200);
  server.begin();

  // Display MAC address and SSID on OLED
  disp.clear();
  disp.drawString(0, 0, "MAC Address:");
  disp.drawString(0, 15, macStr);
  disp.drawString(0, 30, "SSID: " + String(ssid));
  disp.drawString(0, 45, "IP: " + myIP.toString());
  disp.display();

  // Initialize LoRa
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String data = client.readStringUntil('\n');
    Serial.println("Received data: " + data);
    
    // Transmit the received data over LoRa
    if (lora_idle) {
      lora_idle = false;
      data.toCharArray(txpacket, BUFFER_SIZE);
      Serial.printf("Sending packet \"%s\", length %d\r\n", txpacket, strlen(txpacket));
      Radio.Send((uint8_t *)txpacket, strlen(txpacket));
    }
  }
  
  Radio.IrqProcess();
}

void OnTxDone(void) {
  Serial.println("TX done......");
  lora_idle = true;
}

void OnTxTimeout(void) {
  Radio.Sleep();
  Serial.println("TX Timeout......");
  lora_idle = true;
}