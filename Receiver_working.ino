#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- LCD Setup ----------------
LiquidCrystal_I2C lcd(0x27,16,2);

// ---------------- Wi-Fi & MQTT ----------------
const char* ssid = "Dora";
const char* password = "dora@2003";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// ---------------- Wi-Fi Connect ----------------
void connectWiFi(){
  WiFi.begin(ssid,password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
}

// ---------------- MQTT Callback ----------------
String lastTemperature = "--";
String lastAirQuality = "--";

void callback(char* topic, byte* payload, unsigned int length){
  String msg = "";
  for(unsigned int i=0;i<length;i++) msg += (char)payload[i];

  if(String(topic)=="wsn/air-quality/message"){
    lcd.setCursor(0,0);
    lcd.print("Msg: " + msg + "       "); // clear remaining
  } 
  else if(String(topic)=="wsn/air-quality/temperature"){
    lastTemperature = msg;
  } 
  else if(String(topic)=="wsn/air-quality/air"){
    lastAirQuality = msg;
  }

  // Always update row 1 with temp + air
  lcd.setCursor(0,1);
  lcd.print("T:" + lastTemperature + "C Air:" + lastAirQuality + "   ");

  Serial.print("Topic: "); Serial.print(topic);
  Serial.print(" | Message: "); Serial.println(msg);
}

// ---------------- MQTT Reconnect ----------------
void reconnectMQTT(){
  while(!client.connected()){
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32Receiver-" + String(random(0xffff),HEX);
    if(client.connect(clientId.c_str())){
      Serial.println("connected");
      client.subscribe("wsn/air-quality/message");
      client.subscribe("wsn/air-quality/temperature");
      client.subscribe("wsn/air-quality/air");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry 5s");
      delay(5000);
    }
  }
}

// ---------------- Setup ----------------
void setup(){
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Receiver Init");

  connectWiFi();
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);
}

// ---------------- Loop ----------------
void loop(){
  if(!client.connected()) reconnectMQTT();
  client.loop();
}
