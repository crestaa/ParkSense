# ParkSense

ParkSense is an IoT sensor system for parking occupancy and environmental monitoring. It combines parking space detection using time-of-flight sensors with environmental monitoring capabilities for temperature and humidity.

## Project Structure

- `data_listener/`: TypeScript service that processes MQTT messages and stores data in PostgreSQL
- `devices/`: Arduino sketches for the IoT devices
  - `gateway_btm/`: Low-level gateway code for Heltec WiFi LoRa32 v3 (ESP-NOW to LoRa)
  - `gateway_top/`: Top-level gateway code for Heltec WiFi LoRa32 v3 (LoRa to MQTT)
  - `sensor/`: Sensor node code for ESP32 boards
- `mosquitto/`: MQTT broker configuration
- `web_server/`: Dashboard interface for data visualization
- `database/`: PostgreSQL initialization scripts

## Hardware Requirements

- Sensor nodes: ESP32 boards with VL53L0XV2 (ToF) and DHT22 sensors
- Gateways: Heltec WiFi LoRa32 v3 boards
- Appropriate power supplies and enclosures

## Setup Instructions

### Server Deployment

1. Clone the repository
2. Configure environment variables in `docker-compose.yml`
3. Run the server stack:
   ```bash
   docker compose up
   ```
   This will start:
   - PostgreSQL database (port 5432)
   - Mosquitto MQTT broker (port 1883)
   - Data processing service
   - Web dashboard (port 3000)

### Hardware Setup

1. Install Arduino IDE
2. Install required libraries:
   - ESP32 board support
   - Heltec ESP32 board support
   - LoRa
   - ESP-NOW
   - DHT sensor library
   - VL53L0X library

3. Flash the devices:
   - Upload `sensor/main_sensor.ino` to ESP32 boards
   - Upload `gateway_btm/main_gateway_btm.ino` to low-level Heltec gateways
   - Upload `gateway_top/main_gateway_top.ino` to top-level Heltec gateways

4. Configure each device's `config.h` with appropriate network settings and pins

The web interface will be available at `http://server_ip:3000`

## Network Architecture

Sensors → (ESP-NOW) → Low-level Gateway → (LoRa) → Top-level Gateway → (MQTT) → Server
