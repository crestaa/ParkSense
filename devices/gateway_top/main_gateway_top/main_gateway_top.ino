#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h"

#define RF_FREQUENCY 868000000 // Hz
#define LORA_BANDWIDTH 0 // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 12 // [SF7..SF12]
#define LORA_CODINGRATE 4 // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH 8 // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0 // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 3000
#define BUFFER_SIZE 128 // Define the payload size here

// OLED Display
SSD1306Wire disp(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

char rxpacket[BUFFER_SIZE];
static RadioEvents_t RadioEvents;

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

void setup() {
  Serial.begin(115200);
  
  // Initialize OLED display
  disp.init();
  disp.setFont(ArialMT_Plain_10);
  
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  
  RadioEvents.RxDone = OnRxDone;
  
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  // Display initial message
  disp.clear();
  disp.drawString(0, 0, "LoRa Receiver");
  disp.drawString(0, 20, "Listening...");
  disp.display();

  Serial.println("LoRa Receiver initialized. Listening...");
}

void loop() {
  Radio.Rx(0); // Continuous receive mode
  delay(100);
  Radio.IrqProcess();
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  
  Serial.printf("\r\nReceived packet \"%s\" with RSSI %d , SNR %d\r\n", rxpacket, rssi, snr);

  // Display received data on OLED
  disp.clear();
  disp.drawString(0, 0, "Received Data:");
  disp.drawString(0, 15, String(rxpacket));
  disp.drawString(0, 30, "RSSI: " + String(rssi) + " dBm");
  disp.drawString(0, 45, "SNR: " + String(snr) + " dB");
  disp.display();
}