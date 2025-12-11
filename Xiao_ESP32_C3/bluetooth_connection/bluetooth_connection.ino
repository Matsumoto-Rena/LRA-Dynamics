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

  // 受信コールバックの一部
  void onWrite(BLECharacteristic *pCharacteristic) {
      // std::string value = pCharacteristic->getValue(); // もしここがエラーなら String に戻してください
      String value = pCharacteristic->getValue();

      if (value.length() == 3) {
          // 1バイト目: モード (1:絶対, 2:相対)
          int mode = (int)value[0];
          // 2バイト目: X
          int rawX = (int)value[1];
          // 3バイト目: Y
          int rawY = (int)value[2];

          int finalX = 0;
          int finalY = 0;

          if (mode == 1) {
              // --- モード1: 絶対座標 ---
              // 0〜255 がそのまま座標
              finalX = rawX;
              finalY = rawY;
              Serial.printf("絶対座標: X=%d, Y=%d\n", finalX, finalY);

          } else if (mode == 2) {
              // --- モード2: 相対座標 ---
              // 127 が中心なので、引いて「移動量」に戻す
              // 例: 受信127 -> 0 (停止)
              // 例: 受信137 -> +10 (右へ)
              finalX = rawX - 127;
              finalY = rawY - 127;
              Serial.printf("相対移動: dX=%d, dY=%d\n", finalX, finalY);
          }

          // ここでモーター制御などを行う
          // controlMotor(finalX, finalY);
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
