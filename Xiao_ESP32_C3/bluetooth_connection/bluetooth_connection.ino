#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// -----------------------------------------------------------------
// !! 重要 !!
// ブラウザ側の JavaScript と UUID を完全に一致させます
// -----------------------------------------------------------------
#define SERVICE_UUID        "0000aaaa-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000bbbb-0000-1000-8000-00805f9b34fb"
// -----------------------------------------------------------------

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// 接続/切断イベントを処理するコールバック
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("クライアントが接続しました");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("クライアントが切断しました");
      // 切断されたら、再度アドバタイズ（発見可能状態）を開始
      pServer->getAdvertising()->start();
    }
};

// ブラウザからの書き込み(Write)イベントを処理するコールバック
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      // 受信したデータを取得
      String value = pCharacteristic->getValue();

      // シリアルモニタに受信データを表示
      Serial.print("ブラウザからデータを受信: ");
      if (value.length() > 0) {
        for (int i = 0; i < value.length(); i++) {
          Serial.print((int)value[i]); // 10進数の値として表示
          Serial.print(" ");
        }
        Serial.println();

        // TODO: ここで受信データに応じた処理を実行する
        // 例: 76 ('L') を受信したら LED を点灯、など
        // if ((int)value[0] == 76) {
        //   digitalWrite(LED_BUILTIN, HIGH);
        // }
        
      } else {
        Serial.println("データが空です");
      }
    }
};


void setup() {
  Serial.begin(115200);
  Serial.println("BLE サーバーを起動します...");

  // 1. BLEデバイスの初期化
  // "XIAO_ESP32_C3" という名前でアドバタイズ（電波発信）します
  BLEDevice::init("XIAO_ESP32_C3"); 

  // 2. BLEサーバーの作成
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); // 接続・切断コールバックを登録

  // 3. BLEサービスの作成
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. BLE特性 (Characteristic) の作成
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE // ブラウザからの「書き込み」を許可
                    );

  // 5. 特性 (Characteristic) に書き込みコールバックを登録
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // 6. サービスの開始
  pService->start();

  // 7. アドバタイズ（発見可能状態）の開始
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  
  Serial.println("アドバタイズ（待機中）... ブラウザから接続してください。");
}

void loop() {
  // loop() では特に何もしません。
  // すべての処理はコールバック（イベント）によって駆動されます。
  delay(2000);
}
