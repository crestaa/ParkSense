#!/bin/sh
set -e

touch /mosquitto/config/password_file
mosquitto_passwd -b /mosquitto/config/password_file "${MQTT_USERNAME}" "${MQTT_PASSWORD}"

exec /usr/sbin/mosquitto -c /mosquitto/config/mosquitto.conf