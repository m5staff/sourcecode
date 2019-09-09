#include <M5StickC.h>
#include <BLEDevice.h>
#include <BLE2902.h>

#define SERVICE_UUID        "<set your UUID>"
#define CHARACTERISTIC_A_UUID "<set your UUID>"
#define CHARACTERISTIC_B_UUID "<set your UUID>"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristicA = NULL;
BLECharacteristic* pCharacteristicB = NULL;
bool fConnected = false;
uint8_t count = 0;
uint8_t rcvVal = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      M5.Lcd.println("[BLE] Connect");
      fConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      M5.Lcd.println("[BLE] Disconnect");
      fConnected = false;
    }
};

class CharacteristicACallbacks: public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
    M5.Lcd.println("[BLE] Read");
    pCharacteristicA->setValue(&count, 1);
  }  
};

class CharacteristicBCallbacks: public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
    M5.Lcd.println("[BLE] Read");
    pCharacteristicB->setValue(&rcvVal, 1);
  }  

  void onWrite(BLECharacteristic *pCharacteristic) {
    uint8_t *val = pCharacteristicB->getData();
    rcvVal = val[0];
    M5.Lcd.print("[BLE] Write:");
    M5.Lcd.println(rcvVal);
  }
};

// the setup routine runs once when M5StickC starts up
void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.println("[BLE] Start");

  BLEDevice::init("m5stickc");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristicA = pService->createCharacteristic(
                                        CHARACTERISTIC_A_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_NOTIFY
                                      );
  pCharacteristicB = pService->createCharacteristic(
                                        CHARACTERISTIC_B_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_WRITE
                                      );
  pCharacteristicA->setCallbacks(new CharacteristicACallbacks());
  pCharacteristicA->addDescriptor(new BLE2902());
  pCharacteristicB->setCallbacks(new CharacteristicBCallbacks());
  pCharacteristicB->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
}

// the loop routine runs over and over again forever
void loop(){
  if (M5.BtnA.wasPressed()) {
    count++;
    if(fConnected){
      pCharacteristicA->setValue(&count, 1);
      pCharacteristicA->notify();
    }
	}

  M5.update();
}
