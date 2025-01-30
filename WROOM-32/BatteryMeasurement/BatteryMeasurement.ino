#include "../config.h"
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <PubSubClient.h>

// MQTT Broker
const char* StatusTopic = "home/ESP32/Status";
const char* BatteryTopic = "home/ESP32/Battery";
const int mqtt_port = 1883;

// Humidity sensor pin
#define BATTERY_PIN 25


// Sleep duration (in microseconds)
const uint64_t SHORT_SLEEP_DURATION = 60e6;  // 1 minute
const uint64_t LONG_SLEEP_DURATION = SHORT_SLEEP_DURATION;
const int MAX_ATTEMPTS = 20;

WiFiClient espClient;
PubSubClient client(espClient);

long min(long a, long b) {
  return (a < b) ? a : b;
}

// Function to connect to WiFi
void connectToWiFi() {
  // esp_task_wdt_reset();
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  byte retryCount = 0;               // Retry counter stored locally
  unsigned long delayDuration = 10;  // Initial delay duration in milliseconds

  while (WiFi.status() != WL_CONNECTED) {
    delay(delayDuration);

    // Increment retry count
    retryCount++;

    // Exponential backoff with cap
    delayDuration = min(delayDuration * 2, 4000);  // Cap the delay at 4000ms

    // Check if max retries are reached
    if (retryCount > MAX_ATTEMPTS) {
      GoSleep(SHORT_SLEEP_DURATION);
      return;
    }
  }
}


// Function to connect to MQTT broker
void connectToMQTT() {
  //  esp_task_wdt_reset();
  client.disconnect();
  client.setServer(mqtt_broker, mqtt_port);
  byte retryCount = 0;               // Retry counter stored locally
  unsigned long delayDuration = 10;  // Initial delay duration in milliseconds

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      return;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in " + String(delayDuration) + " milliseconds");
    }

    // Increment the retry count
    retryCount++;

    // If max retries reached, go to sleep
    if (retryCount > MAX_ATTEMPTS) {
      Serial.println("Max attempts reached, going to sleep");
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


int getBatteryLevel() {
  int totalValue = 0;
  const int numReadings = 10;

  for (int i = 0; i < numReadings; i++) {
    totalValue += analogRead(BATTERY_PIN);
    delay(10);  // Small delay between readings
  }
  // Calculate the average ADC value
  int averageValue = totalValue / numReadings;

  // Map the ADC value to the desired voltage range (assuming a 12-bit ADC resolution)
  float voltage = (averageValue / 4095.0) * 3300;  // Convert ADC value to millivolts (assuming 3.3V reference)

  // Scale voltage to a percentage between 3000mV and 1500mV
  int percentage = map(voltage, 1500, 3000, 0, 100);

  // Constrain the result between 0% and 100%
  return constrain(percentage, 0, 100);
}

void setup() {
  Serial.begin(115200);
  // // Initialize the Task Watchdog Timer if not already initialized
  // esp_task_wdt_config_t wdt_config = {
  //   .timeout_ms = 300000,  // Set timeout to 5 mins
  //   .trigger_panic = true  // Trigger panic on timeout
  // };
  // esp_task_wdt_init(&wdt_config);

  // esp_err_t status = esp_task_wdt_status(NULL);  // NULL refers to the current task
  // if (status == ESP_ERR_NOT_FOUND) {
  //   // Add the current task to the TWDT
  //   esp_err_t add_status = esp_task_wdt_add(NULL);
  // }

  Serial.println("Starting setup...");
  pinMode(BATTERY_PIN, INPUT);
  int batteryLevel = getBatteryLevel();
  Serial.print("Battery level: ");
  Serial.println(batteryLevel);

  Serial.println("Connecting to WiFi...");
  connectToWiFi();
  Serial.println("Connected to WiFi");

  Serial.println("Connecting to MQTT...");
  connectToMQTT();
  Serial.println("Connected to MQTT");

  // Check the reset reason to determine if this is the first setup
  if (esp_reset_reason() == ESP_RST_POWERON) {
    // If the ESP32 is booting up (not waking from deep sleep)
    Serial.println("Publishing online status...");
    Publish(StatusTopic, "esp32-client-online");
    GoSleep(SHORT_SLEEP_DURATION);
  }
  Serial.println("Publishing battery level...");
  Publish(BatteryTopic, String(batteryLevel).c_str());

  Serial.println("Going to sleep...");
  GoSleep(LONG_SLEEP_DURATION);
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
void loop() {
  // Nothing needed in the loop since we're using deep sleep
}
