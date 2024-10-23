#include <esp_now.h>
#include <WiFi.h>
#include "Wire.h"
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <esp_wifi.h>
#include <esp_system.h>

// OLED Display
SSD1306Wire disp(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// PMK and LMK keys for encryption - must match sensor
uint8_t PMK[] = "PMK1234567891234"; // 16 bytes
uint8_t LMK[] = "LMK1234567891234"; // 16 bytes

char apMacStr[18];
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

void initWiFi() {
    // Initialize WiFi in Station mode first
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    // Then switch to AP mode
    WiFi.mode(WIFI_AP);
    
    // Configure WiFi channel
    wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
    esp_wifi_set_channel(1, secondChan);

    
    // Get and store AP MAC
    uint8_t ap_mac[6];
    esp_wifi_get_mac(WIFI_IF_AP, ap_mac);
    snprintf(apMacStr, sizeof(apMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             ap_mac[0], ap_mac[1], ap_mac[2], ap_mac[3], ap_mac[4], ap_mac[5]);
    Serial.print("Gateway AP MAC: ");
    Serial.println(apMacStr);
}

// ESP-NOW callback when data is received
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
    char macStr[18];
    const uint8_t *mac_addr = recv_info->src_addr;
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    
    // Convert received data to string
    char dataStr[BUFFER_SIZE];
    memcpy(dataStr, data, data_len);
    dataStr[data_len] = '\0';
    
    Serial.printf("\nReceived from: %s\nData: %s\n", macStr, dataStr);
    
    // Forward data over LoRa
    if (lora_idle) {
        lora_idle = false;
        strncpy(txpacket, dataStr, BUFFER_SIZE);
        Serial.printf("Sending packet \"%s\", length %d\r\n", txpacket, strlen(txpacket));
        Radio.Send((uint8_t *)txpacket, strlen(txpacket));
    }
}

void updateDisplay() {
    disp.clear();
    disp.drawString(0, 0, "Gateway AP MAC:");
    
    // Split MAC address into two lines for better readability
    String mac = String(apMacStr);
    String firstHalf = mac.substring(0, 8);
    String secondHalf = mac.substring(9);
    
    disp.drawString(0, 10, mac);
    disp.drawString(0, 50, "Channel: 1");
    disp.display();
}

void setup() {
    Serial.begin(115200);
    
    // Initialize OLED
    disp.init();
    disp.setFont(ArialMT_Plain_10);
    
    // Initialize WiFi and ESP-NOW
    initWiFi();
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    esp_now_register_recv_cb(OnDataRecv);
    
    // Display initial information
    updateDisplay();
    
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
    Radio.IrqProcess();
    // Refresh display periodically to ensure MAC is always visible
    updateDisplay();
    delay(100);  // Small delay to prevent too frequent updates
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