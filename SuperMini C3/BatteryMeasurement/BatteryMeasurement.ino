#include "../config.h"
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <PubSubClient.h>

// MQTT Broker
const char* StatusTopic = "home/ESP32/Status";
const char* BatteryTopic = "home/ESP32/Battery";
const int mqtt_port = 1883;

// Humidity sensor pin
#define BATTERY_PIN 0

const int MAX_ATTEMPTS = 999;

typedef struct {
  int percentage;
  float voltage;
} batteryStatus;

WiFiClient espClient;
PubSubClient client(espClient);

long min(long a, long b) {
  return (a < b) ? a : b;
}

// Function to connect to WiFi
void connectToWiFi() {
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
  }
}


// Function to connect to MQTT broker
void connectToMQTT() {
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


batteryStatus getBatteryLevel() {
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
  Serial.print("Voltage: ");
  Serial.println(voltage);
  // Scale voltage to a percentage between 3000mV and 1500mV
  int percentage = map(voltage, 1500, 3000, 0, 100);

  batteryStatus returnValue = {
    .percentage = constrain(percentage, 0, 100),  // Timeout in milliseconds (240 seconds)
    .voltage = voltage,                           // Trigger a panic (reset) on timeout
  };
  // Constrain the result between 0% and 100%
  return returnValue;
}

String BatteryStatusToString(batteryStatus value) {
  return "Percentage:" + String(value.percentage) + "%\nVoltage: " + String(value.voltage) + "mv";
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

  Serial.println("Starting setup...");
  pinMode(BATTERY_PIN, INPUT);

  Serial.println("Connecting to WiFi...");
  connectToWiFi();
  Serial.println("Connected to WiFi");

  Serial.println("Connecting to MQTT...");
  connectToMQTT();
  Serial.println("Connected to MQTT");
  if (esp_reset_reason() == ESP_RST_POWERON) {
    // If the ESP32 is booting up (not waking from deep sleep)
    Serial.println("Publishing online status...");
    Publish(StatusTopic, "esp32-client-online");
  }
}

void loop() {
  delay(500);
  batteryStatus batteryLevel = getBatteryLevel();
  Serial.print("Battery level: ");
  Serial.println(batteryLevel.percentage);
  Serial.println("Publishing battery level...");
  Publish(BatteryTopic, BatteryStatusToString(batteryLevel).c_str());
}
