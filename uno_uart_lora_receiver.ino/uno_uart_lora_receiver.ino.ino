#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ============================
// Pin Definitions
// ============================
#define LM35_PIN 34        // Analog input for LM35
#define MQ7_PIN 35         // Analog input for MQ7 gas sensor
#define GREEN_LED 25       // BLE connected indicator
#define RED_LED 26         // Gas alert indicator

// ============================
// Objects
// ============================
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust 0x27 if needed (I2C address)

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;

// ============================
// UUIDs
// ============================
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-5678-90ab-cdef-1234567890ab"

// ============================
// BLE Callback Classes
// ============================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    digitalWrite(GREEN_LED, HIGH);
    lcd.setCursor(0,0);
    lcd.print("BLE Connected   "); // extra spaces to overwrite old text
    Serial.println("BLE client connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    digitalWrite(GREEN_LED, LOW);
    lcd.setCursor(0,0);
    lcd.print("BLE Disconnected");
    Serial.println("BLE client disconnected");
    pServer->getAdvertising()->start(); // Restart advertising
  }
};

// ============================
// Setup
// ============================
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Air Monitor + BLE");

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("System Starting...");
  delay(2000);
  lcd.clear();

  // LEDs
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(LM35_PIN, INPUT);
  pinMode(MQ7_PIN, INPUT);

  // Initialize BLE
  BLEDevice::init("ESP32_AirMonitor");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();

  lcd.setCursor(0,0);
  lcd.print("Waiting for BLE");
  Serial.println("Waiting for client connection...");
}

// ============================
// Main Loop
// ============================
void loop() {
  // ==== Read MQ7 ====
  int gasValue = analogRead(MQ7_PIN);

  // ==== Read LM35 ====
  int rawValue = analogRead(LM35_PIN);
  float voltage = (rawValue * 3.3) / 4095.0;  // 12-bit ADC, 3.3V reference
  float temperature = voltage / 0.01;         // 10 mV per °C

  // ==== Display on LCD ====
  lcd.setCursor(0,1);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C   "); // extra spaces to overwrite old digits

  lcd.setCursor(8,1);
  lcd.print("Gas:");
  lcd.print(gasValue);
  lcd.print("   "); // extra spaces to overwrite old digits

  // ==== BLE Send ====
  if (deviceConnected) {
    char data[32];
    snprintf(data, sizeof(data), "T:%.1fC|Gas:%d", temperature, gasValue);
    pCharacteristic->setValue(data);
    pCharacteristic->notify();
  }

  // ==== LED Indicators ====
  if (gasValue > 500) {
    digitalWrite(RED_LED, HIGH);
  } else {
    digitalWrite(RED_LED, LOW);
  }

  Serial.print("Temp: "); Serial.print(temperature,1);
  Serial.print(" C | Gas: "); Serial.println(gasValue);

  delay(3000); // Update every 3 seconds
}
