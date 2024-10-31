#include "../config.h"
#include <WiFi.h>
#include <PubSubClient.h>

// MQTT Broker
const char *StatusTopic = "home/ESP32/Status";
const char *HumidityTopic = "home/ESP32/Humidity";
const char *ControlTopic = "home/ESP32/Control";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);
  // Connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the Wi-Fi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Local MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  // Publish and subscribe
  Publish(StatusTopic,"esp32-client-online");
  client.subscribe(ControlTopic);
}

void callback(char *topic, byte *payload, unsigned int length) {

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");

  // Check the content of the message and control the LED
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
}

void Publish(String topic, const char* payload) {
  // Append the MAC address to the topic
  topic += "/";
  topic += WiFi.macAddress();

  // Publish the message
  client.publish(topic.c_str(), payload);
}

void loop() {
  client.loop();
}

String getMoistureLevel() {
  int WaterValue = analogRead(HUMIDITY_PIN);
  if (value > WaterValue && value < (WaterValue + intervals)) {
    return "Very Wet";
  } else if (value >= (WaterValue + intervals) && value < (AirValue - intervals)) {
    return "Wet";
  } else if (value < AirValue && value >= (AirValue - intervals)) {
    return "Dry";
  } else if (value >= AirValue) {
    return "Very Dry";
  }
  return "Unknown"; // In case value does not fit any range
}

