/*
  SprayShark_BleSniffer.ino
  
  Purpose:
  Capture and print raw BLE data from the Dabble App to the Serial Monitor.
  Use this to find the Hex codes for Gamepad buttons.
*/

#include <ArduinoBLE.h>

// BLE UUIDs (Nordic UART Service - Standard for Dabble/Bluefruit)
const char* serviceUuid    = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* rxCharUuid     = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // RX (Write)
const char* txCharUuid     = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // TX (Notify)

BLEService uartService(serviceUuid);
BLECharacteristic rxCharacteristic(rxCharUuid, BLEWrite | BLEWriteWithoutResponse, 20);
BLECharacteristic txCharacteristic(txCharUuid, BLENotify, 20);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  BLE.setDeviceName("SprayShark");
  BLE.setLocalName("SprayShark");
  
  uartService.addCharacteristic(rxCharacteristic);
  uartService.addCharacteristic(txCharacteristic);
  BLE.addService(uartService);
  BLE.setAdvertisedService(uartService);
  BLE.advertise();

  Serial.println("BLE Sniffer Ready. Connect Dabble App -> Gamepad.");
  Serial.println("Press buttons and copy the HEX output below.");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.println("Connected to Dabble!");

    while (central.connected()) {
      if (rxCharacteristic.written()) {
        int len = rxCharacteristic.valueLength();
        const uint8_t* val = rxCharacteristic.value();
        
        Serial.print("Data: ");
        for (int i = 0; i < len; i++) {
          Serial.print("0x");
          if (val[i] < 16) Serial.print("0");
          Serial.print(val[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
      }
    }
    Serial.println("Disconnected.");
  }
}
