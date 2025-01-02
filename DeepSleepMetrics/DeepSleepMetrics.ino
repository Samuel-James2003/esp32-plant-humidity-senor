#include "../config.h"
#include <WiFi.h>
#include <PubSubClient.h>

// MQTT Broker
const char* StatusTopic = "home/ESP32/Status";
const char* HumidityTopic = "home/ESP32/Humidity";
const int mqtt_port = 1883;

// Humidity sensor pin
#define HUMIDITY_PIN 13

// ADC calibration values
const int AirValue = 3100;
const int WaterValue = 1420;
int intervals = (AirValue - WaterValue) / 3;

// Sleep duration (in microseconds)
const uint64_t SHORT_SLEEP_DURATION = 10e6; // 10 seconds
const uint64_t LONG_SLEEP_DURATION = 36e8;  // 1 hour

WiFiClient espClient;
PubSubClient client(espClient);

// Function to connect to WiFi
void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  byte maxAttempts = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    maxAttempts++;
    if (maxAttempts > 50) {
      Serial.println("Max wifi tries reached");
      GoSleep(SHORT_SLEEP_DURATION);
    }
  }
  Serial.println("\nConnected to WiFi.");
}

// Function to connect to MQTT broker
void connectToMQTT() {
  byte maxAttempts = 0;
  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker.");
    } else if (maxAttempts > 50) {
      Serial.println("Max tries reached");
      GoSleep(SHORT_SLEEP_DURATION);
    } else if (client.state() == -2) {
      maxAttempts++;
      Serial.println("State -2");
      delay(10);
    } else {
      maxAttempts++;
      Serial.print("Failed to connect, state: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// Function to read the moisture level
String getMoistureLevel() {
  int value = analogRead(HUMIDITY_PIN);
  if (value < WaterValue)
    return "0";
  else if (value > AirValue)
    return String(AirValue - WaterValue);

  return String(value - WaterValue);
}


void setup() {
  Serial.begin(115200);
  pinMode(HUMIDITY_PIN, INPUT);
  String moistureLevel = getMoistureLevel();


  connectToWiFi();
  connectToMQTT();

  // Check the reset reason to determine if this is the first setup
  if (esp_reset_reason() == ESP_RST_POWERON) {
    // If the ESP32 is booting up (not waking from deep sleep)
    Serial.println("Setting status to online...");
    Publish(StatusTopic, "esp32-client-online");
    GoSleep(SHORT_SLEEP_DURATION);
  }
  Serial.println("Moisture Level: " + moistureLevel);
  delay(10);
  Publish(HumidityTopic, moistureLevel.c_str());
  delay(10);

  GoSleep(LONG_SLEEP_DURATION);
}
//Put the esp32 in sleep mode
void GoSleep(uint64_t SLEEP_DURATION) {
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION);  // Set the wake-up timer
  esp_deep_sleep_start();                         // Enter deep sleep
  Serial.println("Going to sleep...");
}
//Generic MQTT publish function with delay to ensure upload
void Publish(String topic, const char* payload) {
  // Append the MAC address to the topic
  topic += "/";
  topic += WiFi.macAddress();
  delay(10);
  // Publish the message
  client.publish(topic.c_str(), payload);
  //Wait for message to be sent
  delay(10);
}
void loop() {
  // Nothing needed in the loop since we're using deep sleep
}
