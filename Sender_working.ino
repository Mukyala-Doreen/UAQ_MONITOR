#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>

// ---------------- LoRa UART Setup ----------------
HardwareSerial LoRaSerial(2);
#define LORA_RX 16
#define LORA_TX 17
#define LORA_SET 5

// ---------------- LCD Setup (I2C) ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- WiFi Setup ----------------
const char* ssid = "Dora";
const char* password = "dora@2003";

// ---------------- MQTT Setup ----------------
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

// ---------------- Sensor Pins ----------------
#define TEMP_PIN 34   // LM35 analog pin
#define GAS_PIN 35    // MQ-7 analog pin

// ---------------- LoRa Buffer ----------------
String lineBuffer = "";

// ---------------- Connect to WiFi ----------------
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ---------------- MQTT Callback ----------------
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Display on LCD
  if (String(topic) == "wsn/air-quality/temperature") {
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + message + " C   ");
  } 
  else if (String(topic) == "wsn/air-quality/air") {
    lcd.setCursor(0, 1);
    int airVal = message.toInt();
    lcd.print("Air: " + String((airVal < 100) ? "SAFE " : "UNSAFE") + airVal + "   ");
  }
}

// ---------------- MQTT Reconnect ----------------
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Sender-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("wsn/air-quality/status", "Sender Connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 5s");
      delay(5000);
    }
  }
}

// ---------------- Send Sensor Data ----------------
void sendSensorData() {
  float temperature = analogRead(TEMP_PIN) * (3.3 / 4095.0) * 100; // LM35 conversion
  int gasValue = analogRead(GAS_PIN); // MQ-7 raw value

  // Format line for LoRa
  String message = "Temp=" + String(temperature, 1) + ",Gas=" + String(gasValue);
  LoRaSerial.println(message); // send via LoRa
  Serial.println("Sent via LoRa: " + message);

  // Publish to MQTT
  char tempStr[8], gasStr[8];
  dtostrf(temperature, 4, 1, tempStr);
  itoa(gasValue, gasStr, 10);

  client.publish("wsn/air-quality/temperature", tempStr);
  client.publish("wsn/air-quality/air", gasStr);

  // Display locally
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(temperature, 1) + " C   ");
  lcd.setCursor(0, 1);
  lcd.print("Air: " + String((gasValue < 100) ? "SAFE " : "UNSAFE") + gasValue + "   ");
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(9600);
  pinMode(LORA_SET, OUTPUT);
  digitalWrite(LORA_SET, HIGH);

  // Initialize LoRa UART
  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 LoRa MQTT");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // Initialize WiFi
  connectWiFi();

  // Initialize MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("Setup complete");
}

// ---------------- Loop ----------------
void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Send sensor data every 2 seconds
  sendSensorData();
  delay(2000);

  // Optional: read incoming LoRa data if acting as receiver too
  while (LoRaSerial.available()) {
    char c = LoRaSerial.read();
    if (c == '\n') {
      Serial.println("Received LoRa: " + lineBuffer);
      lineBuffer = "";
    } else {
      lineBuffer += c;
    }
  }
}
