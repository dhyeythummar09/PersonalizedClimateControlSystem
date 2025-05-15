#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// WiFi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// EMQX Cloud MQTT Credentials
const char* mqtt_server = "your-mqtt-broker-url";
const int mqtt_port = 8883;
const char* mqtt_user = "YOUR_MQTT_USERNAME";
const char* mqtt_password = "YOUR_MQTT_PASSWORD";

// MQTT Topics
const char* topic_data = "esp32/sensors";
const char* topic_control = "esp32/control";
const char* topic_occupancy = "esp32/occupancy";

// MQTT Clients
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Sensor & Pin Config
#define DHTPIN 14
#define DHTTYPE DHT22
#define MQ135_PIN 32
#define BUZZER_PIN 27
#define ALERT_LED_PIN 33
#define FAN_LED_PIN 13
#define SERVO_PIN 15
#define AIR_PURIFIER_PIN 26
#define AC_PIN 12

DHT dht(DHTPIN, DHTTYPE);
Servo ventServo;

// Servo positions
const int SERVO_CLOSE_POS = 27;    // 0 degrees for closed
const int SERVO_OPEN_POS = 117;    // 90 degrees for open

// States
String control_mode = "auto";
String fan_status = "off";
String vent_status = "closed";
String air_purifier_status = "off";
String ac_status = "off";
int occupancy = 0;

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
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Connected to EMQX Cloud");
      client.subscribe(topic_control);
      client.subscribe(topic_occupancy);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void turnOffAllDevices() {
  fan_status = "off";
  vent_status = "closed";
  air_purifier_status = "off";
  ac_status = "off";
  
  digitalWrite(FAN_LED_PIN, HIGH);
  ventServo.write(SERVO_CLOSE_POS);
  delay(15); // Give servo time to move
  digitalWrite(AIR_PURIFIER_PIN, LOW);
  digitalWrite(AC_PIN, LOW);
  
  noTone(BUZZER_PIN);
  digitalWrite(ALERT_LED_PIN, LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  if (String(topic) == topic_control) {
    Serial.print("Control message: ");
    Serial.println(msg);

    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, msg);
    if (err) return;

    control_mode = doc["mode"] | "auto";

    if (control_mode == "disabled") {
      turnOffAllDevices();
    }
    else if (control_mode == "manual") {
      String fanCmd = doc["fan"] | "off";
      String ventCmd = doc["vent"] | "closed";
      String purifierCmd = doc["air_purifier"] | "off";
      String acCmd = doc["ac"] | "off";

      fan_status = fanCmd;
      vent_status = ventCmd;
      air_purifier_status = purifierCmd;
      ac_status = acCmd;

      digitalWrite(FAN_LED_PIN, fan_status == "on" ? LOW : HIGH);
      ventServo.write(vent_status == "open" ? SERVO_OPEN_POS : SERVO_CLOSE_POS);
      delay(15); // Give servo time to move
      digitalWrite(AIR_PURIFIER_PIN, air_purifier_status == "on" ? HIGH : LOW);
      digitalWrite(AC_PIN, ac_status == "on" ? HIGH : LOW);
    }
  }
  else if (String(topic) == topic_occupancy) {
    occupancy = msg.toInt();
    Serial.print("Received occupancy update: ");
    Serial.println(occupancy);
    
    if (occupancy <= 0 && control_mode != "disabled") {
      control_mode = "disabled";
      turnOffAllDevices();
    }
    else if(occupancy > 0){
      control_mode = "auto";
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Initialize servo timer
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // Setup servo
  ventServo.setPeriodHertz(50); // Standard 50Hz servo
  ventServo.attach(SERVO_PIN, 500, 2400); // Extended pulse range

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MQ135_PIN, INPUT);
  pinMode(ALERT_LED_PIN, OUTPUT);
  pinMode(FAN_LED_PIN, OUTPUT);
  pinMode(AIR_PURIFIER_PIN, OUTPUT);
  pinMode(AC_PIN, OUTPUT);

  // Initialize all devices to OFF state
  turnOffAllDevices();

  setup_wifi();
  espClient.setInsecure(); // Only for testing without certificates
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int mq135_value = analogRead(MQ135_PIN);

  // Calculate feels-like temperature
  float tempF = temperature * 9.0 / 5.0 + 32.0;
  float R = humidity;
  float heatIndexF = -42.379 + (2.04901523 * tempF) + (10.14333127 * R)
                   + (-0.22475541 * tempF * R) + (-6.83783e-3 * tempF * tempF)
                   + (-5.481717e-2 * R * R) + (1.22874e-3 * tempF * tempF * R)
                   + (8.5282e-4 * tempF * R * R) + (-1.99e-6 * tempF * tempF * R * R);
  float feels_like = (heatIndexF - 32.0) * 5.0 / 9.0;

  Serial.print("Feels Like Temp: ");
  Serial.println(feels_like);

  if (control_mode == "auto") {
    // Fan control
    if (feels_like > 35) {
      fan_status = "on";
      digitalWrite(FAN_LED_PIN, LOW);
    } else {
      fan_status = "off";
      digitalWrite(FAN_LED_PIN, HIGH);
    }

    // AC control
    if (feels_like > 45) {
      ac_status = "on";
      digitalWrite(AC_PIN, HIGH);
    } else {
      ac_status = "off";
      digitalWrite(AC_PIN, LOW);
    }

    // Vent control
    if (mq135_value > 1600) {
      vent_status = "open";
      ventServo.write(SERVO_OPEN_POS);
    } else {
      vent_status = "closed";
      ventServo.write(SERVO_CLOSE_POS);
    }
    delay(15); // Give servo time to move

    // Air purifier control
    if (mq135_value > 1300) {
      air_purifier_status = "on";
      digitalWrite(AIR_PURIFIER_PIN, HIGH);
    } else {
      air_purifier_status = "off";
      digitalWrite(AIR_PURIFIER_PIN, LOW);
    }

    // Buzzer + alert LED
    if (mq135_value > 1700) {
      tone(BUZZER_PIN, 1000);
      digitalWrite(ALERT_LED_PIN, HIGH);
    } else {
      noTone(BUZZER_PIN);
      digitalWrite(ALERT_LED_PIN, LOW);
    }
  } else if (control_mode == "disabled") {
    turnOffAllDevices();
  }

  // Prepare and publish JSON data
  DynamicJsonDocument doc(256);
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["air_quality"] = mq135_value;
  doc["fan_status"] = fan_status;
  doc["vent_status"] = vent_status;
  doc["air_purifier_status"] = air_purifier_status;
  doc["ac_status"] = ac_status;
  doc["mode"] = control_mode;
  doc["occupancy"] = occupancy;

  String payload;
  serializeJson(doc, payload);

  Serial.print("Publishing: ");
  Serial.println(payload);
  client.publish(topic_data, payload.c_str());

  delay(5000);
}
