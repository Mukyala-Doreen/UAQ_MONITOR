/* esp32_uart_lora_sender.ino
   - Reads MQ-7 via ADC + DHT22
   - Sends payload over UART (Serial1) to UART-LoRa module at 9600 baud
   - Assumes LoRa SET tied to GND (data mode), EN tied to 3.3V
*/

#include <Arduino.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ADC pin for MQ-7 voltage divider
#define MQ7_PIN 35

// UART for LoRa: Serial1 (rxPin, txPin)
#define LORA_RX_PIN 16 // module TXD -> ESP32 RX1 (GPIO16)
#define LORA_TX_PIN 17 // module RXD <- ESP32 TX1 (GPIO17)
#define LORA_BAUD 9600

// Optional aux pin
#define LORA_AUX 26

// ADC scaling
const float ADC_REF = 3.3;
const float Rtop = 10000.0;
const float Rbottom = 6800.0;
const float SCALE = (Rtop + Rbottom) / Rbottom;

unsigned long interval = 8000;
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("ESP32 UART-LoRa Sender starting...");

  // start Serial1 for LoRa
  Serial1.begin(LORA_BAUD, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

  pinMode(LORA_AUX, INPUT);
  dht.begin();
}

float readMQ7Voltage() {
  int raw = analogRead(MQ7_PIN);
  float v = (raw / 4095.0) * ADC_REF;
  float sensorV = v * SCALE;
  return sensorV;
}

void loop() {
  if (millis() - lastSend < interval) return;
  lastSend = millis();

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  float mq7_v = readMQ7Voltage();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT read failed.");
    // still send MQ reading
  }

  // Build payload
  char payload[120];
  snprintf(payload, sizeof(payload), "{\"t\":%.2f,\"h\":%.2f,\"g\":%.2f}", temp, hum, mq7_v);

  Serial.print("To LoRa: "); Serial.println(payload);

  // Wait for LoRa AUX idle (optional) — typical: AUX HIGH when idle.
  unsigned long t0 = millis();
  while (digitalRead(LORA_AUX) == LOW) {
    if (millis() - t0 > 500) break; // timeout
    delay(5);
  }

  Serial1.print(payload);    // send to LoRa UART (module will transmit)
  Serial1.print('\n');       // newline (some modules require newline terminator)
}
