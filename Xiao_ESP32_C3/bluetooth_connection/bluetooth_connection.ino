#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_MCP4725.h>

#define SERVICE_UUID        "0000aaaa-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000bbbb-0000-1000-8000-00805f9b34fb"

bool deviceConnected = false;

Adafruit_MCP4725 dac1;

// --- 状態管理 ---
bool isAutoRunning = false; // 自動ループ中か
int manualId = 0;           // 手動実行中のパターンID (0=停止, 1~4=実行中)

// --- パラメータ変数 ---
int paramRes1 = 12;
int paramRep1 = 3;
int paramRes2 = 15;
int paramRep2 = 1;
int loopCount = 200;

// すべてのGPIOピンをLowに
void stopAll() {
  digitalWrite(D2, LOW); digitalWrite(D3, LOW);
  digitalWrite(D6, LOW); digitalWrite(D7, LOW);
  digitalWrite(D8, LOW); digitalWrite(D10, LOW);
  dac1.setVoltage(0, false, 800000);
}

// 接続コールバック
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Connect!");
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Disconnect!");
      pServer->getAdvertising()->start();
    }
};

// 停止指令が来ていないかチェックする関数
// 手動モードで指を離した(manualIdが0になった)ら true を返す
bool checkStop() {
  // 手動モード実行中なのに、通信によって停止(0)に書き換わっていたら中断
  if (manualId == 0 && !isAutoRunning) return true; 
  
  // 自動モード中にSTOPされたら中断
  if (isAutoRunning == false && manualId == 0) return true;

  return false;
}

void move(int res, int repeatCount, bool isDir){
  if(res <= 0) res = 1;
  if(repeatCount < 0) repeatCount = 0;

  // delayMicroseconds(100);
  if(checkStop()) return; // ループの途中でも指が離れたら即終了
  for(int i = 0; i < repeatCount; i++){

    if(isDir){
      digitalWrite(D2,LOW); digitalWrite(D3,HIGH);
    }else{
      digitalWrite(D2,HIGH); digitalWrite(D3,LOW);
    }
    for(uint8_t k=0; k<res; k++){
      dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
    }
  }

  // delayMicroseconds(100);
  if(checkStop()) return;

  if(isDir){
    digitalWrite(D2,HIGH); digitalWrite(D3,LOW);
  }else{
    digitalWrite(D2,LOW); digitalWrite(D3,HIGH);
  }
  for(uint8_t k=0; k<res; k++){
    dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
  }
}

void move2(int res, int repeatCount, bool isDir){
  if(res <= 0) res = 1;
  if(repeatCount < 0) repeatCount = 0;

  delayMicroseconds(100);
  for(int i = 0; i < repeatCount; i++){
  if(checkStop()) return; // ループの途中でも指が離れたら即終了

    if(isDir){
      digitalWrite(D2,LOW); digitalWrite(D3,HIGH);
    }else{
      digitalWrite(D2,HIGH); digitalWrite(D3,LOW);
    }
    for(uint8_t k=0; k<res; k++){
      dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
    }
  }

  delayMicroseconds(100);
  if(checkStop()) return;
  
  if(isDir){
    digitalWrite(D2,HIGH); digitalWrite(D3,LOW);
  }else{
    digitalWrite(D2,LOW); digitalWrite(D3,HIGH);
  }
  for(uint8_t k=0; k<res; k++){
    dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
  }
}

// 指定したパターンの設定で1単位だけ動かす
void runPatternStep(int id) {
  // ピン設定
  digitalWrite(D6, LOW); digitalWrite(D8, LOW); digitalWrite(D7, LOW); digitalWrite(D10, LOW);
  
  switch(id) {
    case 1: digitalWrite(D6, HIGH);  for(uint8_t i=0; i < 10; i++) move(paramRes1, paramRep1, true);  break;
    case 2: digitalWrite(D8, HIGH);  for(uint8_t i=0; i < 10; i++) move(paramRes1, paramRep1, false); break;
    case 3: digitalWrite(D7, HIGH);  move2(paramRes2, paramRep2, true);  break;
    case 4: digitalWrite(D10, HIGH); move2(paramRes2, paramRep2, false); break;
  }
}

// 書き込みコールバック
// 書き込みコールバック
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      // 生データを取得
      uint8_t* data = pCharacteristic->getData();
      
      // ★修正: std::string ではなく String を使います
      String valueStr = pCharacteristic->getValue();
      int len = valueStr.length();

      if (len == 3) {
        int header = data[0]; // 1バイト目: モード識別
        int val1   = data[1]; // 2バイト目
        int val2   = data[2]; // 3バイト目

        if (header == 0x01) {
            // --- モード1: 手動 ---
            isAutoRunning = false;
            manualId = val1;
        } else if (header == 0x02) {
            // --- モード2: LRA自動追尾 ---
            isAutoRunning = true;
        }
      }
    }
};

void setup() {
  // MDのGPIOピン設定
  pinMode(D2,OUTPUT); pinMode(D3,OUTPUT);
  pinMode(D6,OUTPUT); pinMode(D7,OUTPUT);
  pinMode(D8,OUTPUT); pinMode(D10,OUTPUT);
  // dacの設定
  dac1.begin(0x62);

  stopAll();

  Serial.begin(115200);
  BLEDevice::init("XIAO_ESP32_C3");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  Serial.println("Waiting for connection...");
}

void loop() {
  // --- 手動モード実行中 (manualIdが1~4の間) ---
  if (manualId > 0) {
    runPatternStep(manualId);
    // runPatternStepの中でcheckStop()しているので、指を離せば次回ループでmanualId=0になり止まる
    return; 
  }
}