#include <M5Stack.h>
#include <BLEDevice.h>

static BLEUUID serviceUUID("<set your UUID>");
static BLEUUID charAUUID("<set your UUID>");
static BLEUUID charBUUID("<set your UUID>");

static BLEAdvertisedDevice* targetDevice;
static BLEClient*  pClient;
static BLERemoteCharacteristic* pRemoteCharacteristicA;
static BLERemoteCharacteristic* pRemoteCharacteristicB;
static boolean doConnect = false;
bool fConnected = false;
uint8_t count = 0;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertised_device) {
    if (advertised_device.haveServiceUUID() && advertised_device.isAdvertisingService(serviceUUID)) {
      M5.Lcd.println("[BLE] Found service");
      M5.Lcd.println("[BLE] Scan stop");
      BLEDevice::getScan()->stop();
      targetDevice = new BLEAdvertisedDevice(advertised_device);
      doConnect = true;
    }
  }
};

class MyTargetCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    fConnected = false;
    M5.Lcd.println("[BLE] Disconnect");
  }
};

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    M5.Lcd.print("[BLE] Notify :");
    M5.Lcd.println(pData[0]);
}

bool connectToServer() {    
  pClient  = BLEDevice::createClient();

  pClient->setClientCallbacks(new MyTargetCallback());
  pClient->connect(targetDevice);

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == NULL) {
    M5.Lcd.println("[BLE] Not found my service.");
    pClient->disconnect();
    return false;
  }
  fConnected = true;

  pRemoteCharacteristicA = pRemoteService->getCharacteristic(charAUUID);
  if (pRemoteCharacteristicA == NULL) {
    pClient->disconnect();
    return false;
  }
  M5.Lcd.println("[BLE] Found our characteristicA");

  if(pRemoteCharacteristicA->canNotify())
      pRemoteCharacteristicA->registerForNotify(notifyCallback);

  pRemoteCharacteristicB = pRemoteService->getCharacteristic(charBUUID);
  if (pRemoteCharacteristicA == NULL) {
    pClient->disconnect();
    return false;
  }
  
  M5.Lcd.println("[BLE] Found our characteristicB");
  return true;
}

void setup() {
  M5.begin();
  M5.Lcd.println("[BLE] Start");

  BLEDevice::init("");
  BLEScan* ble_scan = BLEDevice::getScan();
  ble_scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  ble_scan->setInterval(1349);
  ble_scan->setWindow(449);
  ble_scan->setActiveScan(true);
  ble_scan->start(5, false);
}

void loop(){
  if (doConnect == true) {
    if (connectToServer()) {
      M5.Lcd.println("[BLE] Connect");
    } else {
      M5.Lcd.println("[BLE] Error on connect");
    }
    doConnect = false;
  }
  
  if(fConnected) {
    if (M5.BtnA.wasPressed()) {
      uint8_t value = pRemoteCharacteristicA->readUInt8();
      M5.Lcd.print("[BLE] Read :");
      M5.Lcd.println(value);
    }

    if (M5.BtnB.wasPressed()) {
      count++;
      pRemoteCharacteristicB->writeValue(&count, 1);
      M5.Lcd.print("[BLE] Write :");
      M5.Lcd.println(count);
    }

    if (M5.BtnC.wasPressed()) {
      M5.Lcd.println("[BLE] Disconnect");
        pClient->disconnect();
    }
  }

  M5.update();
}
