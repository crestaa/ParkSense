#!/bin/sh
set -e

# Create password file
touch /mosquitto/config/password_file
mosquitto_passwd -b /mosquitto/config/password_file "${MQTT_USERNAME}" "${MQTT_PASSWORD}"

# Start Mosquitto
exec /usr/sbin/mosquitto -c /mosquitto/config/mosquitto.conf