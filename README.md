# ESP32 MQTT Humidity Monitoring System

## Overview
This project uses an ESP32 to monitor soil humidity levels via a moisture sensor and publish the data to an MQTT broker. The device connects to a WiFi network, sends humidity readings, and enters deep sleep to conserve energy. It also features exponential backoff for retries and watchdog functionality for system reliability.

## Features
- **WiFi Connectivity**: Connects to a WiFi network using credentials from a configuration file.
- **MQTT Publishing**: Sends data to specific MQTT topics for integration with IoT systems.
- **Deep Sleep**: Uses deep sleep mode to extend battery life.
- **Exponential Backoff**: Implements retry mechanisms for robust WiFi and MQTT connections.
- **Watchdog Timer**: Ensures the system resets in case of unexpected failures.
- **Calibrated Humidity Readings**: Calibrates the sensor values for accurate measurements.

## Hardware Requirements
- ESP32 development board
- Soil moisture sensor
- WiFi network
- MQTT broker

## MQTT Topics
- **StatusTopic**: `home/ESP32/Status`
  - Publishes device status messages (e.g., online status).
- **HumidityTopic**: `home/ESP32/Humidity`
  - Publishes soil humidity readings.

## Pin Configuration
- **HUMIDITY_PIN**: GPIO 13

## Software Configuration
1. Create a `config.h` file and define the following variables:
   ```cpp
   const char* ssid = "<Your WiFi SSID>";
   const char* password = "<Your WiFi Password>";
   const char* mqtt_broker = "<Your MQTT Broker Address>";
   const char* mqtt_username = "<Your MQTT Username>";
   const char* mqtt_password = "<Your MQTT Password>";
   ```

## How It Works
1. **WiFi Connection**:
   - The ESP32 connects to the WiFi network.
   - Implements exponential backoff for retries.
   - Goes into short deep sleep after 50 failed attempts.

2. **MQTT Connection**:
   - Connects to the MQTT broker with credentials.
   - Uses exponential backoff for connection retries.

3. **Moisture Reading**:
   - Reads analog values from the sensor.
   - Maps the values to calibrated moisture levels.

4. **Publishing Data**:
   - Publishes the device status and humidity level to MQTT topics.

5. **Deep Sleep**:
   - Puts the ESP32 into deep sleep for a configurable duration to conserve power.

## Function Breakdown
- **`connectToWiFi()`**:
  Handles WiFi connection with retry logic and exponential backoff.
- **`connectToMQTT()`**:
  Manages MQTT connection with retry logic and delay.
- **`getMoistureLevel()`**:
  Calibrates and returns the soil moisture level.
- **`Publish()`**:
  Publishes messages to MQTT topics.
- **`GoSleep()`**:
  Puts the ESP32 into deep sleep mode for a specified duration.

## Deep Sleep Durations
- **Short Sleep**: 10 seconds (used during retries)
- **Long Sleep**: 1 hour (used after successful execution)

## Setup
1. Connect the soil moisture sensor to GPIO 13.
2. Flash the ESP32 with the provided code.
3. Monitor the device via the serial output (baud rate: 115200).
4. Verify MQTT messages in the broker's topics.

## Usage
- Ensure the `config.h` file contains valid WiFi and MQTT credentials.
- Deploy the ESP32 in the field with the soil moisture sensor.
- Monitor MQTT topics for humidity data and device status.

## Notes
- Adjust `AirValue` and `WaterValue` in the code for sensor calibration.
- Ensure the MQTT broker is accessible from the ESP32's network.

---

### Author
Samuel JAMES
