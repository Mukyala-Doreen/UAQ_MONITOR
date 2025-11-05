
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ==== Pin Definitions ====
#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ7_PIN 36
#define GREEN_LED 25
#define RED_LED 26

// ==== Objects ====
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// BLE UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-5678-90ab-cdef-1234567890ab"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

float temperature = 0.0;
float humidity = 0.0;
int gasValue = 0;
float lastTemp = 0, lastHum = 0;

void setup() {
  Serial.begin(9600);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("FireBeetle BLE");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();

  // Initialize sensors
  pinMode(MQ7_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  dht.begin();

  // Initialize BLE
  BLEDevice::init("FireBeetle_Sensor");
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("BLE Initialized - ready to broadcast");
}

void loop() {
  // Read sensors
  gasValue = analogRead(MQ7_PIN);
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Handle DHT error
  if (isnan(temperature) || isnan(humidity)) {
    temperature = lastTemp;
    humidity = lastHum;
    Serial.println("DHT Error!");
  } else {
    lastTemp = temperature;
    lastHum = humidity;
  }

  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gas:");
  lcd.print(gasValue);
  lcd.print("ppm");
  lcd.setCursor(0, 1);
  lcd.print(temperature, 1);
  lcd.print("C ");
  lcd.print(humidity, 1);
  lcd.print("%");

  // LED indication
  if (gasValue > 500) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
  } else {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  }

  // Send data via BLE
  String data = "Gas:" + String(gasValue) +
                " Temp:" + String(temperature, 1) +
                "C Hum:" + String(humidity, 1) + "%";
  pCharacteristic->setValue(data.c_str());
  pCharacteristic->notify(); // Notify BLE client
  Serial.println("BLE Sent: " + data);

  delay(3000);
}
