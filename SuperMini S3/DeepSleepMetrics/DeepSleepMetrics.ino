#include "../config.h"
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <PubSubClient.h>

// MQTT Broker
const char* StatusTopic = "home/ESP32/Status";
const char* HumidityTopic = "home/ESP32/Humidity";
const int mqtt_port = 1883;

// Humidity sensor pin
#define HUMIDITY_PIN 10


// Sleep duration (in microseconds)
const uint64_t SHORT_SLEEP_DURATION = 3e8;  // 5 minutes
const uint64_t LONG_SLEEP_DURATION = 36e8;   // 1 hour
const int MAX_ATTEMPTS = 20;

WiFiClient espClient;
PubSubClient client(espClient);

long min(long a, long b) {
  return (a < b) ? a : b;
}

// Function to connect to WiFi
void connectToWiFi() {
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  byte retryCount = 0;                // Retry counter stored locally
  unsigned long delayDuration =  10; // Initial delay duration in milliseconds

  while (WiFi.status() != WL_CONNECTED) {
    delay(delayDuration);

    // Increment retry count
    retryCount++;

    // Exponential backoff with cap
    delayDuration = min(delayDuration * 2, 4000); // Cap the delay at 4000ms

    // Check if max retries are reached
    if (retryCount > MAX_ATTEMPTS) {
      GoSleep(SHORT_SLEEP_DURATION);
      return;
    }
  }
}


// Function to connect to MQTT broker
void connectToMQTT() {
  client.disconnect();
  client.setServer(mqtt_broker, mqtt_port);
  byte retryCount = 0;                // Retry counter stored locally
  unsigned long delayDuration = 10;  // Initial delay duration in milliseconds

  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      return;
    }

    // Increment the retry count
    retryCount++;

    // If max retries reached, go to sleep
    if (retryCount > MAX_ATTEMPTS) {
      GoSleep(SHORT_SLEEP_DURATION);
      return;
    }

    // Exponential backoff with delay cap
    delay(delayDuration);
    if (delayDuration < 2000) {
      delayDuration *= 2;  // Double the delay duration
      if (delayDuration > 2000) {
        delayDuration = 2000;  // Cap the delay at 2000ms
      }
    }
  }
}


// Function to read the moisture level
int getMoistureLevel() {
  // ADC calibration values
  const int AirValue = 3100;
  const int WaterValue = 1220;
  int intervals = (AirValue - WaterValue) / 3;

  int totalValue = 0;
  const int numReadings = 10;

  for (int i = 0; i < numReadings; i++) {
    totalValue += analogRead(HUMIDITY_PIN);
    delay(10); // Small delay between readings
  }

  int averageValue = totalValue / numReadings;
  return map(averageValue, WaterValue, AirValue, 0, AirValue - WaterValue);
}

//Put the esp32 in sleep mode
void GoSleep(uint64_t SLEEP_DURATION) {
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION);  // Set the wake-up timer
  esp_deep_sleep_start();                         // Enter deep sleep
}
//Generic MQTT publish function with delay to ensure upload
void Publish(String topic, const char* payload) {
  // Append the MAC address to the topic
  topic += "/";
  topic += WiFi.macAddress();
  delay(100);
  // Publish the message
  client.publish(topic.c_str(), payload);
  //Wait for message to be sent
  delay(100);
}

void setup() {
  Serial.begin(115200);      
  pinMode(HUMIDITY_PIN, INPUT);

  connectToWiFi();
  connectToMQTT();

  // Check the reset reason to determine if this is the first setup
  if (esp_reset_reason() == ESP_RST_POWERON) {
    Serial.print(String(getMoistureLevel()));
    // If the ESP32 is booting up (not waking from deep sleep)
    Publish(StatusTopic, "esp32-client-online");
    GoSleep(SHORT_SLEEP_DURATION);
  }
  Publish(HumidityTopic, String(getMoistureLevel()).c_str());

  GoSleep(LONG_SLEEP_DURATION);
}
void loop() {
  // Nothing needed in the loop since we're using deep sleep
}
