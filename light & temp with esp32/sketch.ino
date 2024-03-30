#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHTesp.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "172.187.230.56";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

const int LDRPin = 34;        // GPIO pin number for LDR (analog input)
const int mqttLedPin = 33;    // GPIO pin where LED controlled by MQTT is connected
const int ldrLedPin = 2;      // GPIO pin where LED controlled by LDR is connected
const int DHT_PIN = 27;       // GPIO pin number for DHT22
const int pinR = 17;          // GPIO 17 for red color
const int pinG = 16;          // GPIO 16 for green color
const int pinB = 5;           // GPIO 5 for blue color

DHTesp dht;
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String stMessage = "";
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    stMessage += (char)message[i];
  }
  Serial.println();

  // Check if the message is for controlling the RGB LED
  if (String(topic) == "light/color") {
    // Parse the JSON message
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, message, length);
    if (error) {
      Serial.println("Failed to parse JSON payload");
      return;
    }

    // Extract color values from JSON
    int redValue = doc["red"];
    int greenValue = doc["green"];
    int blueValue = doc["blue"];
    
    // Invert color values
    redValue = invertColor(redValue);
    greenValue = invertColor(greenValue);
    blueValue = invertColor(blueValue);
    
    // Set LED color
    setColor(redValue, greenValue, blueValue);
  }

  if (String(topic) == "website/status") {
    Serial.print("Changing MQTT-controlled LED to ");
    if (stMessage == "on") {
      Serial.println("on");
      digitalWrite(mqttLedPin, HIGH);
    } else if (stMessage == "off") {
      Serial.println("off");
      digitalWrite(mqttLedPin, LOW);
    }
  }


}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("connected");
      client.subscribe("website/status");
      client.subscribe("light/color");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(mqttLedPin, OUTPUT);
  pinMode(LDRPin, INPUT);
  pinMode(ldrLedPin, OUTPUT); // Adding pinMode for LDR LED
  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinB, OUTPUT);

  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  dht.setup(DHT_PIN, DHTesp::DHT22); // Initialize DHT22 sensor
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read LDR sensor and control the LED based on its value
  int LDRStatus = analogRead(LDRPin);
  digitalWrite(ldrLedPin, LDRStatus <= 500 ? HIGH : LOW); // Turn on LED if light is low
  
  // Publish LDR sensor data
  String ldrData = String(LDRStatus);
  client.publish("website/ldrsensor/data", ldrData.c_str());

  // Read temperature and humidity from DHT22 sensor
  TempAndHumidity dhtData = dht.getTempAndHumidity();
  if (!isnan(dhtData.temperature)) {
    String tempData = String(dhtData.temperature);
    client.publish("website/temperature", tempData.c_str());
    Serial.print("Temperature: ");
    Serial.println(tempData);
  } else {
    Serial.println("Failed to read temperature from DHT sensor!");
  }

  if (!isnan(dhtData.humidity)) {
    String humidityData = String(dhtData.humidity);
    client.publish("website/humidity", humidityData.c_str());
    Serial.print("Humidity: ");
    Serial.println(humidityData);
  } else {
    Serial.println("Failed to read humidity from DHT sensor!");
  }

  delay(1000); // Adjust delay as needed
}

void setColor(int redValue, int greenValue, int blueValue) {
  analogWrite(pinR, redValue);
  analogWrite(pinG, greenValue);
  analogWrite(pinB, blueValue);
}

int invertColor(int value) {
  return 255 - value;
}
