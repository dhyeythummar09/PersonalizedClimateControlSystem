#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// WiFi Credentials - Replace with your own
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// EMQX Cloud MQTT Credentials - Replace with your own
const char* mqtt_server = "YOUR_MQTT_BROKER_URL";
const int mqtt_port = 8883; // Port 8883 for secure MQTT over TLS
const char* mqtt_user = "YOUR_MQTT_USERNAME";
const char* mqtt_password = "YOUR_MQTT_PASSWORD";

// MQTT Topics (Retained as per original)
const char* topic_occupancy = "esp32/occupancy";

// MQTT Clients
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Pin definitions - Customize based on your hardware setup
const int irSensor1Pin = YOUR_IR_SENSOR1_PIN;  
const int irSensor2Pin = YOUR_IR_SENSOR2_PIN;  
const int ledPin = YOUR_LED_PIN;              

// Variables for tracking occupancy
int peopleCount = 0;
int sensor1State = HIGH;
int sensor2State = HIGH;
int lastSensor1State = HIGH;
int lastSensor2State = HIGH;
unsigned long lastDetectionTime = 0;
const int detectionTimeout = 1000; // Timeout in milliseconds
bool sensor1FirstTriggered = false;
bool sensor2FirstTriggered = false;

void setup_wifi() {
  delay(100);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32OccupancyClient", mqtt_user, mqtt_password)) {
      Serial.println("Connected to MQTT Broker");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void publishOccupancy() {
  String payload = String(peopleCount);
  client.publish(topic_occupancy, payload.c_str());
  Serial.print("Published occupancy: ");
  Serial.println(payload);
}

void resetDetection() {
  sensor1FirstTriggered = false;
  sensor2FirstTriggered = false;
}

void updateOccupancy(String action) {
  Serial.print(action);
  Serial.print(" - Current occupancy: ");
  Serial.println(peopleCount);
  publishOccupancy();
}

void setup() {
  Serial.begin(115200);

  // Set pin modes
  pinMode(irSensor1Pin, INPUT);
  pinMode(irSensor2Pin, INPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, LOW); // Turn off LED initially

  setup_wifi();
  espClient.setInsecure(); // ⚠️ Only for testing without certificates
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Room Occupancy Tracker Initialized");
  Serial.println("Current occupancy: 0");

  publishOccupancy(); // Initial publish
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  sensor1State = digitalRead(irSensor1Pin);
  sensor2State = digitalRead(irSensor2Pin);

  if (sensor1FirstTriggered || sensor2FirstTriggered) {
    if (millis() - lastDetectionTime > detectionTimeout) {
      resetDetection();
    }
  }

  if (sensor1State != lastSensor1State && sensor1State == LOW && !sensor2FirstTriggered) {
    lastDetectionTime = millis();
    sensor1FirstTriggered = true;
    Serial.println("Sensor 1 triggered - possible entry");
  }

  if (sensor2State != lastSensor2State && sensor2State == LOW && !sensor1FirstTriggered) {
    lastDetectionTime = millis();
    sensor2FirstTriggered = true;
    Serial.println("Sensor 2 triggered - possible exit");
  }

  if (sensor1FirstTriggered && sensor2State != lastSensor2State && sensor2State == LOW) {
    peopleCount++;
    updateOccupancy("Person entered");
    resetDetection();
  }

  if (sensor2FirstTriggered && sensor1State != lastSensor1State && sensor1State == LOW) {
    if (peopleCount > 0) {
      peopleCount--;
    }
    updateOccupancy("Person exited");
    resetDetection();
  }

  lastSensor1State = sensor1State;
  lastSensor2State = sensor2State;

  digitalWrite(ledPin, peopleCount > 0 ? HIGH : LOW);

  delay(50); // Debounce delay
}
