#include "../config.h"
#include <DHT.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SPIFFS.h"
#include <time.h>

// MQTT Information
const char* StatusTopic = "home/ESP32/Status";
const char* SensorTopic = "home/ESP32/Sensor";
const char* DebugTopic = "home/ESP32/Debug";
const int mqtt_port = 1883;

//Structures
struct SensorData {
  float temperature;
  float heat_index;
  float air_humidity;
  int battery;
  int soil_humidity;
  char timestamp[30];
};

// Definitions
#define HUMIDITY_PIN 0
#define DHT_PIN 1
#define BATTERY_PIN 2
#define DHTTYPE DHT22
#define TIMEZONE "EST5EDT,M3.2.0,M11.1.0"
#define COMPLETED true
#define FAILED false


const uint64_t SLEEP_DURATION = 5e6;//36e8;   //Sleep duration (in microseconds) 1 hour
const int MAX_ATTEMPTS = 50;
const int MAX_DELAY = 2000;

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
    delayDuration = min(delayDuration * 2, MAX_DELAY);  // Cap the delay at MAX_DELAY

    // Check if max retries are reached
    if (retryCount > MAX_ATTEMPTS) {
      GoSleep(FAILED);
      return;
    }
  }
}

// Function to connect to MQTT broker
void connectToMQTT() {
  client.disconnect();
  client.setServer(mqtt_broker, mqtt_port);
  byte retryCount = 0;               // Retry counter stored locally
  unsigned long delayDuration = 10;  // Initial delay duration in milliseconds

  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      return;
    }

    // Increment the retry count
    retryCount++;

    // If max retries reached, go to sleep
    if (retryCount > MAX_ATTEMPTS) {
      GoSleep(FAILED);
      return;
    }

    // Exponential backoff with delay cap
    delay(delayDuration);
    if (delayDuration < MAX_DELAY) {
      delayDuration *= 2;  // Double the delay duration
      if (delayDuration > MAX_DELAY) {
        delayDuration = MAX_DELAY;  // Cap the delay at 2000ms
      }
    }
  }
}

// Configure time with NTP and timezone
void getTime(SensorData& data) {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", TIMEZONE, 1);
  tzset();
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  strcpy(data.timestamp, buffer);
}

// Function to read the moisture level
void getMoistureLevel(SensorData& data) {
  // ADC calibration values
  const int AirValue = 3100;
  const int WaterValue = 1420;
  int averageValue = getSensorInfo(HUMIDITY_PIN);
  int moistureLevel = map(averageValue, WaterValue, AirValue, 0, AirValue - WaterValue);
  data.soil_humidity = constrain(moistureLevel, 0, AirValue - WaterValue);
}

void getDHTLevels(SensorData& data) {
    DHT dht(DHT_PIN, DHTTYPE);
    dht.begin();
    float airHumidity;
    float temperature;
    
    // Keep reading until we get valid values
    do {
        airHumidity = dht.readHumidity();
        temperature = dht.readTemperature();
    } while (isnan(airHumidity) || isnan(temperature));
    
    float heatIndex = dht.computeHeatIndex(temperature, airHumidity, false);
    
    data.heat_index = heatIndex;
    data.air_humidity = airHumidity;
    data.temperature = temperature;
}

int getSensorInfo(int pinNumber) {
  int totalValue = 0;
  const int numReadings = 10;

  for (int i = 0; i < numReadings; i++) {
    totalValue += analogRead(pinNumber);
    delay(10);  // Small delay between readings
  }

  return totalValue / numReadings;
}

void getBatteryLevel(SensorData& data) {
  int value = getSensorInfo(BATTERY_PIN);
  float voltage = (value / 4095.0) * 3300;  // Convert ADC value to millivolts (assuming 3.3V reference)
  // Scale voltage to a percentage between 3000mV and 1500mV
  int percentage = map(voltage, 1500, 3000, 0, 100);
  data.battery = constrain(percentage, 0, 100);
}

void structToJson(const SensorData& data, JsonDocument& doc) {
  doc["temperature"] = data.temperature;
  doc["air_humidity"] = data.air_humidity;
  doc["battery"] = data.battery;
  doc["soil_humidity"] = data.soil_humidity;
  doc["timestamp"] = data.timestamp;
  doc["heat_index"] = data.heat_index;
}

void resendStoredData() {
  if (!client.connected()) return;

  File file = SPIFFS.open("/sensor_data.json", "r");
  if (!file) {
    Serial.println("No stored data found");
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    // Publish the line as a JSON string
    if (!Publish(SensorTopic, line.c_str())) break;
  }
  file.close();

  // Clear the file after successful transmission
  SPIFFS.remove("/sensor_data.json");
}

void saveDataToFile(String jsonData) {
  // Open the file for appending data
  File file = SPIFFS.open("/sensor_data.json", "a");  // Open in append mode
  if (!file) {
    return;
  }

  // Write JSON data to file
  file.println(jsonData);
  file.close();

  Serial.println("Data saved to SPIFFS");
}

//Put the esp32 in sleep mode
void GoSleep(bool isComplete) {
  if (isComplete) {
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION);  // Set the wake-up timer
    esp_deep_sleep_start();                         // Enter deep sleep
    return;
  }
  saveDataToFile(CollectData());
}

String CollectData(){
  SensorData data;
  getBatteryLevel(data);
  getDHTLevels(data);
  getMoistureLevel(data);
  getTime(data);

  // Create a JSON document
  StaticJsonDocument<200> doc;

  // Convert struct to JSON
  structToJson(data, doc);
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

//Generic MQTT publish function with delay to ensure upload
bool Publish(String topic, const char* payload) {
  // Append the MAC address to the topic
  topic += "/";
  topic += WiFi.macAddress();
  delay(100);
  // Publish the message
  bool success = client.publish(topic.c_str(), payload);
  //Wait for message to be sent
  delay(100);
  return success;
}

void setup() {
  Serial.begin(115200);
  pinMode(HUMIDITY_PIN, INPUT);
  pinMode(BATTERY_PIN, INPUT);

  connectToWiFi();
  connectToMQTT();

  // Check the reset reason to determine if this is the first setup
  if (esp_reset_reason() == ESP_RST_POWERON) {
    // If the ESP32 is booting up (not waking from deep sleep
    Publish(StatusTopic, "esp32-client-online");
  }
  String jsonString = CollectData();
  Serial.println(jsonString);

  //Publish
  resendStoredData();
  Publish(SensorTopic, jsonString.c_str());

  //Go to sleep
  GoSleep(COMPLETED);
}

void loop() {
  // Nothing needed in the loop since we're using deep sleep
}
