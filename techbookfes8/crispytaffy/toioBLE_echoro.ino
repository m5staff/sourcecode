#include <M5Stack.h>
#include <BLEDevice.h>

// toio自体, モーターなどをBLE Peripheralとして発見するためにService UUIDを設定
// M5側がtoioのadvertiseを受けて検出できる
// toio service
static BLEUUID serviceUUID("10B20100-5B3B-4571-9508-CF3EFCD7BBAE");
// motor characteristic
static BLEUUID motor_charUUID("10B20102-5B3B-4571-9508-CF3EFCD7BBAE");
// sound characteristic
static BLEUUID sound_charUUID("10B20104-5B3B-4571-9508-CF3EFCD7BBAE");

static BLEAdvertisedDevice* targetDevice;
static BLEClient*  pClient;
static BLERemoteCharacteristic* motor_characteristic;
static BLERemoteCharacteristic* sound_characteristic;
static boolean doConnect = false;
static boolean doScan = false;

static boolean is_req_move = false;
static boolean is_req_sound = false;

bool fConnected = false;

// toioキューブの制御に必要な構成データ
static uint8_t motor_data_size = 8;
static uint8_t sound_data_size = 15;
static uint8_t current_sound = 0;

// Ultrasonic SensorをM5Stackの21番ピンに接続
const int SigPin = 21;
long duration;
int distance;

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertised_device) {
    if (advertised_device.haveServiceUUID() && advertised_device.isAdvertisingService(serviceUUID)) {
      Serial.println("[BLE] Found service");
      Serial.println("[BLE] Scan stop");
      BLEDevice::getScan()->stop();
      targetDevice = new BLEAdvertisedDevice(advertised_device);
      doConnect = true;
      doScan = true;
    } // Found our server
  } // onResult
}; // Myadvertised_deviceCallbacks

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    fConnected = true;
    Serial.println("onConnect");
  }

  void onDisconnect(BLEClient* pClient) {
    fConnected = false;
    Serial.println("onDisconnect");
  }
};

// 超音波センサーで10cm以内に近づいたら前進する
static void moveMotor(void)
{
  static bool is_near = false;
  is_near = distance;
  if (distance < 10){
    M5.Lcd.drawString("MOVE", 0, 180);
    is_req_move = true;
  }else{
    M5.Lcd.drawString("STOP", 0, 180);
    is_req_move = false;
  }
}

// 時間指定付きモーター制御で送信されるデータ
// data[1]-[3]で左のタイヤ、data[4]-[6]で右のタイヤを制御
static void sendMotorControl(void)
{
  uint8_t data[motor_data_size] = {0};
  data[0] = 0x02; //時間指定付きモーター制御
  data[1] = 0x01; //左　motor id 1 : left
  data[2] = 0x01; //前
  data[3] = (is_req_move ? 0x64 : 0x00); //モーターの速度指示値　0x64 = 100
  data[4] = 0x02; //右　motor id 2 : right
  data[5] = 0x02; //後
  data[6] = (is_req_move ? 0x64 : 0x00); //モーターの速度指示値　0x1E = 30
  data[7] = 0x64; //モータの制御時間, 100ミリ秒
  
  motor_characteristic->writeValue(data, sizeof(data));
}

static void sendSoundControl(void)
{
  uint8_t data[sound_data_size] = {0};
  data[0] = 0x03; //0x03:MIDI note number
  data[1] = 0x02; //繰り返しの回数 1回
  data[2] = (is_req_move ? 0x04 : 0x0); //Operationの数 4つ
  data[3] = 0x1E; //再生時間 300ミリ秒
  data[4] = 0x3C; //MIDI note number C5
  data[5] = 0xFF; //音量 最大
  data[6] = 0x1E; //再生時間 300ミリ秒
  data[7] = 0x40; //MIDI note number E5
  data[8] = 0xFF; //音量 最大
  data[9] = 0x1E; //再生時間 300ミリ秒
  data[10] = 0x43; //MIDI note number G5
  data[11] = 0xFF; //音量 最大
  data[12] = 0x1E; //再生時間 300ミリ秒
  data[13] = 0x40; //MIDI note number E5
  data[14] = 0xFF; //音量 最大

  sound_characteristic->writeValue(data, sizeof(data));
}

bool connectToServer() {
    pClient  = BLEDevice::createClient();    
//    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(targetDevice);  // if you pass BLEadvertised_device instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* remote_service = pClient->getService(serviceUUID);
    if (remote_service == NULL) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    fConnected = true;
    Serial.println(" - Found toio service");
    Serial.println(uxTaskGetStackHighWaterMark(NULL));

    //motor
    motor_characteristic = remote_service->getCharacteristic(motor_charUUID);
    if (motor_characteristic == NULL) {
      Serial.print("Failed to find motor characteristic UUID: ");
      Serial.println(motor_charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found motor characteristic");

    //sound
    sound_characteristic = remote_service->getCharacteristic(sound_charUUID);
    if (sound_characteristic == NULL) {
      Serial.print("Failed to find sound characteristic UUID: ");
      Serial.println(sound_charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found sound characteristic");
    
    return true;
//    fConnected = true;
}

void setup() {
  //初期設定
  M5.begin();
  M5.Lcd.fillScreen(0x0000);
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(0xFFFF, 0x0000);

  Serial.print(115200);
  M5.Lcd.println("[BLE] Start");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* ble_scan = BLEDevice::getScan();
  ble_scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  ble_scan->setInterval(1349);
  ble_scan->setWindow(449);
  ble_scan->setActiveScan(true);
  ble_scan->start(5, false);

  // 測距センサーに接続したピンのモードを出力にセット
  pinMode(SigPin, OUTPUT);
  digitalWrite(SigPin, LOW);

} // End of setup.

// This is the Arduino main loop function.
void loop() {

  M5.update();

  // 超音波センサー
  pinMode(SigPin, OUTPUT);
  digitalWrite(SigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(SigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(SigPin, LOW);

  pinMode(SigPin, INPUT);
  duration = pulseIn(SigPin, HIGH);
  distance= duration*0.034/2;
  Serial.println("Distance: " + String(distance) + " cm");
  delay(2000);
  
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      M5.Lcd.println("[BLE] Connect");
    } else {
      M5.Lcd.println("[BLE] Error on connect");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (fConnected) {
//    checkDistance();
    moveMotor();
    sendMotorControl();
    sendSoundControl();
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(1000); // Delay a second between loops.
} // End of loop
