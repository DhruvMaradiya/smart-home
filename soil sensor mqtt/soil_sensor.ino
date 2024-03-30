#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "172.187.230.56";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

const int servoPin = 12;
const int soilSensorPin = 34;

WiFiClient espClient;
PubSubClient client(espClient);

Servo servo;
bool servoState = false;
bool mqttControl = true; // Default to true to automatically start the servo when soil is dry

LiquidCrystal_I2C lcd(0x27, 16, 2);

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (strcmp(topic, "servo/control") == 0) {
    if (strcmp(message, "on") == 0) {
      mqttControl = true;
      if (!servoState) {
        servo.attach(servoPin, 500, 2400);
        servoState = true;
        // Move the servo when turned on
        for (int pos = 0; pos <= 180; pos += 1) {
          servo.write(pos);
          delay(15);
        }
      }
    } else if (strcmp(message, "off") == 0) {
      mqttControl = false;
      if (servoState) {
        servo.detach();
        servoState = false;
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("connected");
      client.subscribe("servo/control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  Wire.begin(23, 22);
  lcd.init();
  lcd.backlight();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int16_t soilMoisture = analogRead(soilSensorPin);
  String soilStatus = soilMoisture < 2165 ? "WET" : soilMoisture > 3135 ? "DRY" : "OK";

  if (soilStatus == "DRY" && mqttControl) {
    if (servoState) {
    for (int pos = 0; pos <= 180; pos += 1) {
      servo.write(pos);
      delay(15);
    }
    for (int pos = 180; pos >= 0; pos -= 1) {
      servo.write(pos);
      delay(15);
    }
  }
  } else {
    if (servoState) {
      servo.detach();
      servoState = false;
    }
  }

  lcd.clear();
  lcd.print("Soil: ");
  lcd.print(soilStatus);
  delay(500);
}
