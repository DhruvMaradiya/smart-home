#include <Stepper.h>
#include <PubSubClient.h>
#include <WiFi.h>

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT broker
const char* mqttServer = "172.187.230.56";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

// MQTT topics
const char* topicManual = "machine/manual";


WiFiClient espClient;
PubSubClient client(espClient);

// Define the number of steps per revolution for stepper motors
const int stepsPerRevolution = 200; // Change if needed based on your stepper motor specs

// Initialize stepper motors with the number of steps and pins connected to them
Stepper stepper1(stepsPerRevolution, 12, 14, 27, 26); // Pin numbers for stepper1 (washing machine)
Stepper stepper2(stepsPerRevolution, 18, 19, 17, 16); // Pin numbers for stepper2 (dryer)

// Define pins for ultrasonic sensor
const int trigPin = 0; // Trigger pin of ultrasonic sensor
const int echoPin = 2; // Echo pin of ultrasonic sensor

// Define constants for distance thresholds
const int washingDistance = 390; // Distance threshold to trigger washing process (in cm)

// Define buzzer pin
const int buzzerPin = 33;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Check if the message is related to manual control
  if (strcmp(topic, topicManual) == 0) {
    // Convert payload to integer
    int command = atoi((char*)payload);
    // Start processes if the command is 1
    if (command == 1) {
      startProcesses();
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
      // Once connected, subscribe to the manual topic
      client.subscribe(topicManual);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // Set up stepper motors
  stepper1.setSpeed(60); // RPM (adjust as needed)
  stepper2.setSpeed(60); // RPM (adjust as needed)

  // Set up ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Set up buzzer pin
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Measure distance using ultrasonic sensor
  long duration;
  digitalWrite(trigPin, LOW); // Ensure trigger pin is low before sending pulse
  delayMicroseconds(2); // Wait for 2 microseconds
  digitalWrite(trigPin, HIGH); // Send a 10 microsecond pulse to trigger
  delayMicroseconds(10); // Wait for 10 microseconds
  digitalWrite(trigPin, LOW); // Stop the pulse
  duration = pulseIn(echoPin, HIGH); // Read the echo duration
  long distance = duration * 0.034 / 2; // Calculate distance in cm

  // Debugging: Print distance to serial monitor
  Serial.print("Cloth in Container: ");
  Serial.print(distance / 4);
  Serial.println(" %");

  // Check if distance threshold for washing is reached
  if (distance >= washingDistance) { // Check if distance is within valid range
    // Start processes
    startProcesses();
  }
}

void startProcesses() {
  // Beep the buzzer 2 times
  beep(buzzerPin, 1000, 200); // Beep with a frequency of 1000 Hz for 200 milliseconds, twice

  // Washing process
  Serial.println("Starting washing process...");
  wash();

  // Delay between washing and drying
  delay(7000); // 2 seconds delay

  // Drying process
  Serial.println("Starting drying process...");
  dry();

  // Beep the buzzer 2 times
  beep(buzzerPin, 1000, 200); // Beep with a frequency of 1000 Hz for 200 milliseconds, twice
}

void wash() {
  // Rotate the washing stepper motor for 5 seconds
  stepper1.step(stepsPerRevolution * 5); // Rotate for 5 seconds (adjust as needed)
}

void dry() {
  // Rotate the drying stepper motor for 5 seconds
  stepper2.step(stepsPerRevolution * 5); // Rotate for 5 seconds (adjust as needed)
}

// Function to make the buzzer beep
void beep(int buzzerPin, unsigned int frequency, unsigned long duration) {
  // Calculate the period of the beep in microseconds
  unsigned long period = 1000000 / frequency;

  // Calculate the number of cycles for the given duration
  unsigned long cycles = frequency * duration / 1000;

  // Toggle the buzzer pin ON and OFF for the calculated duration
  for (unsigned long i = 0; i < cycles; i++) {
    digitalWrite(buzzerPin, HIGH);
    delayMicroseconds(period / 2);
    digitalWrite(buzzerPin, LOW);
    delayMicroseconds(period / 2);
  }
}
