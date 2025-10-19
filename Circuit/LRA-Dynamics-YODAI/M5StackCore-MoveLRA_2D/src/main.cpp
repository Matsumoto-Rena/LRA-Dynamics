#include <Arduino.h>
#include "M5Unified.h"
#include <cmath> // PI を使うために cmath をインクルード

// LRA1 (前後) の制御ピン
int D1_1 = 21;
int D1_2 = 22;
// LRA2 (左右) の制御ピン
int D2_1 = 18;
int D2_2 = 19;
// DAC出力ピン
int A1 = 26;
// サイン波の半周期の分解能
const int length = 85;

// 現在の動作モードを管理するための変数
enum Mode { OFF, RIGHT_ASYMMETRIC, LEFT_ASYMMETRIC, FRONT_ASYMMETRIC, BACK_ASYMMETRIC };
Mode current_mode = Mode::OFF;
Mode last_mode = Mode::OFF;

/**
 * @brief LRAを振動させる関数
 * @param pin_pos 正方向のピン
 * @param pin_neg 負方向のピン
 * @param asymmetric trueなら非対称(3:1)、falseなら対称(1:1)で振動
 */
void vibrate(int pin_pos, int pin_neg, bool asymmetric) {
    int positive_reps = asymmetric ? 3 : 1;

    // 正方向への振動
    for (int i = 0; i < positive_reps; ++i) {
        digitalWrite(pin_neg, LOW);
        digitalWrite(pin_pos, HIGH);
        for (int j = 0; j < length; ++j) {
            dacWrite(A1, 255 * sin(j * PI / length));
        }
    }
    
    // 負方向への振動
    digitalWrite(pin_pos, LOW);
    digitalWrite(pin_neg, HIGH);
    for (int j = 0; j < length; ++j) {
        dacWrite(A1, 255 * sin(j * PI / length));
    }
}

void setup(){
    M5.begin();
    M5.Lcd.begin();
    pinMode(D1_1, OUTPUT);
    pinMode(D1_2, OUTPUT);
    pinMode(D2_1, OUTPUT);
    pinMode(D2_2, OUTPUT);
    pinMode(A1, OUTPUT);
    Serial.begin(9600);
    M5.Lcd.setTextSize(2);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Let's start!");
}

void loop(){
    M5.update();

    // 1. ボタンの状態から現在のモードを決定
    if (M5.BtnC.isPressed()) {
        if (M5.BtnA.isPressed()) { // Aボタン: LRA1 (前後)
            current_mode = M5.BtnB.isPressed() ? Mode::FRONT_ASYMMETRIC : Mode::BACK_ASYMMETRIC;
        } else { // Aボタンなし: LRA2 (左右)
            current_mode = M5.BtnB.isPressed() ? Mode::RIGHT_ASYMMETRIC : Mode::LEFT_ASYMMETRIC;
        }
    } else {
        current_mode = Mode::OFF;
    }

    // 2. モードが変更された瞬間にだけ画面を更新
    if (current_mode != last_mode) {
        M5.Lcd.fillRect(0, 20, M5.Lcd.width(), 16, BLACK);
        M5.Lcd.setCursor(0, 20);
        switch (current_mode) {
            case Mode::FRONT_ASYMMETRIC:   M5.Lcd.println("Front (Asym)"); break;
            case Mode::BACK_ASYMMETRIC:  M5.Lcd.println("Back  (Asym)"); break;
            case Mode::RIGHT_ASYMMETRIC:    M5.Lcd.println("Right (Asym)"); break;
            case Mode::LEFT_ASYMMETRIC:   M5.Lcd.println("Left (Asym)"); break;
            default: break; // OFFの時は何も表示しない
        }
        last_mode = current_mode;
    }

    // 3. 現在のモードに応じてLRAを駆動
    switch (current_mode) {
        case Mode::FRONT_ASYMMETRIC:   vibrate(D1_1, D1_2, true); break;
        case Mode::BACK_ASYMMETRIC:  vibrate(D1_2, D1_1, true);  break;
        case Mode::RIGHT_ASYMMETRIC:    vibrate(D2_1, D2_2, true); break;
        case Mode::LEFT_ASYMMETRIC:   vibrate(D2_2, D2_1, true);  break;
        case Mode::OFF:
            digitalWrite(D1_1, LOW);
            digitalWrite(D1_2, LOW);
            digitalWrite(D2_1, LOW);
            digitalWrite(D2_2, LOW);
            dacWrite(A1, 0);
            break;
    }
}