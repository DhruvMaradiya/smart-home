#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "172.187.230.56";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqttTopic = "dustbin/percentage";

WiFiClient espClient;
PubSubClient client(espClient);

int Trig_pin = 5;
int Echo_pin = 18;
int Buzzer_pin = 26; // Buzzer pin
long duration;
float Speed_of_sound = 0.034;
float dist_in_cm;
const float MAX_DISTANCE_CM = 400.0;  // Maximum distance in centimeters
const float MIN_DISTANCE_CM = 2.0;    // Minimum distance in centimeters

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
  // Handle incoming messages here
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
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
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  pinMode(Trig_pin, OUTPUT);
  pinMode(Echo_pin, INPUT);
  pinMode(Buzzer_pin, OUTPUT); // Set buzzer pin as output
}

void loop() {
  digitalWrite(Trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(Trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trig_pin, LOW);

  duration = pulseIn(Echo_pin, HIGH);

  dist_in_cm = duration * Speed_of_sound / 2;

  // Convert distance to percentage
  float percentage = map(dist_in_cm, MIN_DISTANCE_CM, MAX_DISTANCE_CM, 0, 100);

  Serial.print("Distance in cm: ");
  Serial.println(dist_in_cm);
  Serial.print("Percentage: ");
  Serial.println(percentage);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  char msg[10];
  sprintf(msg, "%.2f", percentage);
  client.publish(mqttTopic, msg);

  // Check if percentage is 100% and trigger buzzer for 5 seconds
  if (percentage >= 99) {
    digitalWrite(Buzzer_pin, HIGH); // Sound the buzzer
    delay(5000); // Sound for 5 seconds
    digitalWrite(Buzzer_pin, LOW); // Turn off the buzzer
  }

  delay(1000);
}
