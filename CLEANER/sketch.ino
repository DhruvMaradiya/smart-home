// Dhruv Maradiya
// 29/03/2024

#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define TRIG_PIN_FRONT 12
#define ECHO_PIN_FRONT 14
#define TRIG_PIN_LEFT 27
#define ECHO_PIN_LEFT 26
#define TRIG_PIN_RIGHT 18
#define ECHO_PIN_RIGHT 17
#define TRIG_PIN_BACK 5
#define ECHO_PIN_BACK 16

#define DIST_THRESHOLD 20

#define BUZZER_PIN 4

Servo servoLeft;
Servo servoRight;

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "172.187.230.56";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* machineTopic = "cleaner/machine";

WiFiClient espClient;
PubSubClient client(espClient);

bool machineState = false;
bool bladeState = false;

void setup() {
  Serial.begin(9600);

  servoLeft.setPeriodHertz(50);
  servoRight.setPeriodHertz(50);

  servoLeft.attach(21, 1000, 2000);
  servoRight.attach(19, 1000, 2000);

  pinMode(TRIG_PIN_FRONT, OUTPUT);
  pinMode(ECHO_PIN_FRONT, INPUT);
  pinMode(TRIG_PIN_LEFT, OUTPUT);
  pinMode(ECHO_PIN_LEFT, INPUT);
  pinMode(TRIG_PIN_RIGHT, OUTPUT);
  pinMode(ECHO_PIN_RIGHT, INPUT);
  pinMode(TRIG_PIN_BACK, OUTPUT);
  pinMode(ECHO_PIN_BACK, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

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
  Serial.println("Message arrived on topic:");
  Serial.print(topic);
  Serial.println();

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Message received:");
  Serial.println(message);

  if (strcmp(topic, machineTopic) == 0) {
    if (message == "on") {
      machineState = true;
      Serial.println("Machine turned ON");
    } else if (message == "off") {
      machineState = false;
      Serial.println("Machine turned OFF");
    } else if (message == "blade_on") {
      bladeState = true;
      Serial.println("Blade turned ON");
    } else if (message == "blade_off") {
      bladeState = false;
      Serial.println("Blade turned OFF");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // Once connected, subscribe to the topics
      client.subscribe(machineTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (machineState) {
    int distanceFront = getDistance(TRIG_PIN_FRONT, ECHO_PIN_FRONT);
    int distanceLeft = getDistance(TRIG_PIN_LEFT, ECHO_PIN_LEFT);
    int distanceRight = getDistance(TRIG_PIN_RIGHT, ECHO_PIN_RIGHT);
    int distanceBack = getDistance(TRIG_PIN_BACK, ECHO_PIN_BACK);

    Serial.print("Front distance: ");
    Serial.print(distanceFront);
    Serial.print(" cm, Left distance: ");
    Serial.print(distanceLeft);
    Serial.print(" cm, Right distance: ");
    Serial.print(distanceRight);
    Serial.print(" cm, Back distance: ");
    Serial.print(distanceBack);
    Serial.println(" cm");

    // Check if all sides have obstacles within 2 cm
    if (distanceFront <= 2 && distanceLeft <= 2 && distanceRight <= 2 && distanceBack <= 2) {
      Serial.println("Obstacles detected on all sides. Stopping machine.");
      machineState = false;
      moveStop(); // Add a function to stop the machine
      return; // Exit the loop to prevent further execution
    }

    if (distanceFront <= DIST_THRESHOLD) {
      if (distanceLeft > distanceRight) {
        Serial.println("Obstacle on the left, turning right");
        turnRight();
      } else {
        Serial.println("Obstacle on the right, turning left");
        turnLeft();
      }
    } else if (distanceBack <= DIST_THRESHOLD) {
      Serial.println("Obstacle behind, turning around");
      turnAround();
    } else {
      Serial.println("No obstacle detected. Moving forward.");
      moveForward();
    }

    // Control the blade if it's turned on
    if (bladeState) {
      // Blade operation logic here
    }

    // Continuous sweeping of both servos
    sweepServos();
  }

  delay(1000);

  digitalWrite(BUZZER_PIN, HIGH);
  delay(100); // Adjust the duration of the beep
  digitalWrite(BUZZER_PIN, LOW);
}

void moveStop() {
  servoLeft.write(90);
  servoRight.write(90);
}


int getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;

  // If the distance is out of range, return a large value
  if (distance <= 0 || distance > 400) {
    return 400; // Maximum sensor range is typically around 400 cm
  }

  return distance;
}

void moveForward() {
  servoLeft.write(90);
  servoRight.write(90);
}

void turnLeft() {
  servoLeft.write(0);
  servoRight.write(180);
}

void turnRight() {
  servoLeft.write(180);
  servoRight.write(0);
}

void turnAround() {
  servoLeft.write(180);
  servoRight.write(180);
}

void sweepServos() {
  for (int pos = 0; pos <= 180; pos += 1) {
    servoLeft.write(pos);
    servoRight.write(pos);
    delay(2);
  }
  for (int pos = 180; pos >= 0; pos -= 1) {
    servoLeft.write(pos);
    servoRight.write(pos);
    delay(2);
  }
}
