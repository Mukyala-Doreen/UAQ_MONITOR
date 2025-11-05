#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ====== LoRa (E32 433T) UART Setup ======
#define LORA_RX 16   // LoRa TX -> ESP32 RX2
#define LORA_TX 17   // LoRa RX <- ESP32 TX2
#define LORA_AUX 33  // Optional (status pin)
HardwareSerial LoRaSerial(2); // Use UART2

// ====== LCD I2C Setup ======
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ====== LED Indicators ======
#define GREEN_LED 25
#define RED_LED 26

String incomingData = "";

void setup() {
  Serial.begin(9600);
  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(LORA_AUX, INPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("LoRa Receiver");
  delay(2000);
  lcd.clear();

  Serial.println("ESP32 LoRa Receiver Ready");
}

void loop() {
  // Check if data is available from LoRa
  if (LoRaSerial.available()) {
    incomingData = LoRaSerial.readStringUntil('\n');
    incomingData.trim();

    Serial.println("Received: " + incomingData);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Received Data:");
    lcd.setCursor(0, 1);
    lcd.print(incomingData);

    // Parse or detect gas level keywords
    if (incomingData.indexOf("Gas:") != -1) {
      int gasStart = incomingData.indexOf("Gas:") + 4;
      int gasEnd = incomingData.indexOf('|', gasStart);
      int gasValue = incomingData.substring(gasStart, gasEnd).toInt();

      if (gasValue > 500) {
        digitalWrite(RED_LED, HIGH);
        digitalWrite(GREEN_LED, LOW);
      } else {
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED, LOW);
      }
    }
  }
  delay(100);
}
