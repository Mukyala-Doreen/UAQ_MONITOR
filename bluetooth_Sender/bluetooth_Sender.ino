#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================
// WiFi & MQTT CONFIGURATION
// ============================
const char* ssid = "Dora";           // Your Wi-Fi SSID
const char* password = "dor33nv1c";  // Your Wi-Fi password
const char* mqtt_server = "airquality.mosquitto.org"; // MQTT broker
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// ============================
// Pin Definitions
// ============================
#define LM35_PIN 34
#define MQ7_PIN 35
#define GREEN_LED 25
#define RED_LED 26

// ============================
// Objects
// ============================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============================
// MQTT RECONNECT FUNCTION
// ============================
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("esp32/status", "ESP32 connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds...");
      delay(5000);
    }
  }
}

// ============================
// SETUP
// ============================
void setup() {
  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Starting...");
  delay(1500);
  lcd.clear();

  // Pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  // ============================
  // Wi-Fi INIT with timeout
  // ============================
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connecting");
  Serial.print("Connecting to WiFi");

  const int WIFI_TIMEOUT = 20000; // 20 seconds
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.print("WiFi Connected");
    Serial.println("\nWiFi connected, IP: " + WiFi.localIP().toString());
    digitalWrite(GREEN_LED, HIGH);
  } else {
    lcd.clear();
    lcd.print("WiFi Failed");
    Serial.println("\nFailed to connect WiFi");
    digitalWrite(RED_LED, HIGH);
  }

  // ============================
  // Simple internet connectivity test
  // ============================
  IPAddress testIP;
  if (WiFi.hostByName("google.com", testIP)) {
    Serial.println("Internet connection: OK");
    lcd.setCursor(0, 1);
    lcd.print("Internet OK    ");
  } else {
    Serial.println("Internet connection: FAILED");
    lcd.setCursor(0, 1);
    lcd.print("Internet FAIL  ");
  }

  // MQTT INIT
  client.setServer(mqtt_server, mqtt_port);
}

// ============================
// MAIN LOOP
// ============================
void loop() {
  // Ensure MQTT is connected
  if (!client.connected()) reconnectMQTT();
  client.loop();

  // Read sensors
  int gasValue = analogRead(MQ7_PIN);
  int rawValue = analogRead(LM35_PIN);
  float voltage = (rawValue * 3.3) / 4095.0;
  float temperature = voltage / 0.01;

  // LCD Display
  lcd.setCursor(0,1);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C ");
  lcd.setCursor(8,1);
  lcd.print("G:");
  lcd.print(gasValue);
  lcd.print("   ");

  // MQTT Publish
  char mqttData[64];
  snprintf(mqttData, sizeof(mqttData), "{\"temperature\":%.1f,\"gas\":%d}", temperature, gasValue);
  client.publish("esp32/sensors", mqttData);

  // LED alert
  if (gasValue > 500) {
    digitalWrite(RED_LED, HIGH);
    client.publish("esp32/alerts", "Gas Level HIGH!");
  } else {
    digitalWrite(RED_LED, LOW);
  }

  // Serial output
  Serial.print("Temp: ");
  Serial.print(temperature, 1);
  Serial.print("°C | Gas: ");
  Serial.println(gasValue);

  delay(3000);
}
