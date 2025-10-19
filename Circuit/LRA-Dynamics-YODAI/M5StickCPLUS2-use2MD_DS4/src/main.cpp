/*
https://robohanactive.slack.com/archives/C08RF0U7A49/p1759132342633789
↑動画をチャンネルにあげてます.
M5StickCPLUS2とDS4を接続し,
ボタン入力によって2つのLRAを対象振動させることができます.
*/
#include <Arduino.h>
#include <M5Unified.h>
#include <Bluepad32.h>
#include <cmath> // PI を使うために cmath をインクルード

// LRA1 (前後) の制御ピン
int D1_1 = 32;
int D1_2 = 33;
// LRA2 (左右) の制御ピン
int D2_1 = 0;
int D2_2 = 25;
// DAC出力ピン
int A1 = 26;
// サイン波の半周期の分解能
const int length = 85;

int cnt = 0;
uint8_t button = 0;

// 現在の動作モードを管理するための変数
enum Mode { OFF, RIGHT_ASYMMETRIC, LEFT_ASYMMETRIC, FRONT_ASYMMETRIC, BACK_ASYMMETRIC };
Mode current_mode = Mode::OFF;
Mode last_mode = Mode::OFF;

// /**
//  * @brief LRAを振動させる関数
//  * @param pin_pos 正方向のピン
//  * @param pin_neg 負方向のピン
//  * @param asymmetric trueなら非対称(3:1)、falseなら対称(1:1)で振動
//  */
// void vibrate(int pin_pos, int pin_neg, bool asymmetric) {
//     int positive_reps = asymmetric ? 3 : 1;

//     // 正方向への振動
//     for (int i = 0; i < positive_reps; ++i) {
//         digitalWrite(pin_neg, LOW);
//         digitalWrite(pin_pos, HIGH);
//         for (int j = 0; j < length; ++j) {
//             dacWrite(A1, 255 * sin(j * PI / length));
//         }
//     }
    
//     // 負方向への振動
//     digitalWrite(pin_pos, LOW);
//     digitalWrite(pin_neg, HIGH);
//     for (int j = 0; j < length; ++j) {
//         dacWrite(A1, 255 * sin(j * PI / length));
//     }
// }


ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
            // Additionally, you can get certain gamepad properties like:
            // Model, VID, PID, BTAddr, flags, etc.
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                           properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not found empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

void dumpGamepad(ControllerPtr ctl) {
    Serial.printf(
        "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
        "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
        ctl->index(),        // Controller Index
        ctl->dpad(),         // D-pad
        ctl->buttons(),      // bitmask of pressed buttons
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->brake(),        // (0 - 1023): brake button
        ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons(),  // bitmask of pressed "misc" buttons
        ctl->gyroX(),        // Gyro X
        ctl->gyroY(),        // Gyro Y
        ctl->gyroZ(),        // Gyro Z
        ctl->accelX(),       // Accelerometer X
        ctl->accelY(),       // Accelerometer Y
        ctl->accelZ()        // Accelerometer Z
    );
}

void processGamepad(ControllerPtr ctl) {
    // There are different ways to query whether a button is pressed.
    // By query each button individually:
    //  a(), b(), x(), y(), l1(), etc...
    button = 0;
    if (ctl->a()) {//aボタン
        button = button | 0b1;
        // Serial.println("a");
    }

    if (ctl->b()) {//bボタン
        button = button | 0b10;
        // Serial.println("b");
    }

    if (ctl->x()) {//xボタン
        button = button | 0b100;
        // Serial.println("x");
    }
    
    if (ctl->y()) {//yボタン
        button = button | 0b1000;
        // Serial.println("y");
    }

    if (ctl->r2()) {//r2ボタン
      button = button | 0b10000;
        // Serial.println("r2");
    }
}

void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            } else {
                Serial.println("Unsupported controller");
            }
        }
    }
}

// Arduino setup function. Runs in CPU 1
void setup() {
    M5.begin();
    M5.Lcd.begin();
    pinMode(D1_1, OUTPUT);
    pinMode(D1_2, OUTPUT);
    pinMode(D2_1, OUTPUT);
    pinMode(D2_2, OUTPUT);
    pinMode(A1, OUTPUT);
    M5.Lcd.setTextSize(2);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Let's start!");

    Serial.begin(115200);
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    // Setup the Bluepad32 callbacks
    BP32.setup(&onConnectedController, &onDisconnectedController);

    // "forgetBluetoothKeys()" should be called when the user performs
    // a "device factory reset", or similar.
    // Calling "forgetBluetoothKeys" in setup() just as an example.
    // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
    // But it might also fix some connection / re-connection issues.
    BP32.forgetBluetoothKeys();

    // Enables mouse / touchpad support for gamepads that support them.
    // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
    // - First one: the gamepad
    // - Second one, which is a "virtual device", is a mouse.
    // By default, it is disabled.
    BP32.enableVirtualDevice(false);
}

// Arduino loop function. Runs in CPU 1.
void loop() {
    M5.update();

    // This call fetches all the controllers' data.
    // Call this function in your main loop.
    bool dataUpdated = BP32.update();
    if (dataUpdated)
        processControllers();

    // The main loop must have some kind of "yield to lower priority task" event.
    // Otherwise, the watchdog will get triggered.
    // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
    // Detailed info here:
    // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

    //     vTaskDelay(1);
    

    for (int j = 0; j < length; ++j) {
        dacWrite(A1, 255 * sin(j * PI / length));
    }
    cnt++;
    if(cnt % 2 == 0){
        if(button & 0b1){//×ボタン押されていたら
            digitalWrite(D1_2, LOW);
            digitalWrite(D1_1, HIGH);
        }else if((button & 0b10)>>1){//〇ボタン押されていたら
            digitalWrite(D1_1, LOW);
            digitalWrite(D1_2, HIGH);
        }else{
            digitalWrite(D1_1, LOW);
            digitalWrite(D1_2, LOW);
        }
    }else{
        if(button & 0b1){//aボタン押されていたら
            digitalWrite(D1_1, LOW);
            digitalWrite(D1_2, HIGH);
        }else if((button & 0b10)>>1){//bボタン押されていたら
            digitalWrite(D1_2, LOW);
            digitalWrite(D1_1, HIGH);
        }else{
            digitalWrite(D1_1, LOW);
            digitalWrite(D1_2, LOW);
        }
    }
    if(cnt % 2 == 0){
        if((button&0b100)>>2){//□ボタン押されていたら
            digitalWrite(D2_2, LOW);
            digitalWrite(D2_1, HIGH);
        }else if((button&0b1000)>>3){//△ボタン押されていたら
            digitalWrite(D2_1, LOW);
            digitalWrite(D2_2, HIGH);
        }else{
            digitalWrite(D2_1, LOW);
            digitalWrite(D2_2, LOW);
        }
    }else{
        if((button&0b100)>>2){//xボタン押されていたら
            digitalWrite(D2_1, LOW);
            digitalWrite(D2_2, HIGH);
        }else if((button&0b1000)>>3){//yボタン押されていたら
            digitalWrite(D2_2, LOW);
            digitalWrite(D2_1, HIGH);
        }else{
            digitalWrite(D2_1, LOW);
            digitalWrite(D2_2, LOW);
        }
    }


}