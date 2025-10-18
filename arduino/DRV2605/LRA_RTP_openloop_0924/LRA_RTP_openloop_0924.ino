#include <Wire.h>

#define DRV_ADDR 0x5A

// --- DRV2605 Register Addresses ---
#define REG_MODE 0x01
#define REG_RTP_INPUT 0x02
#define REG_CONTROL3 0x1D

// --- LRA Parameters ---
#define LRA_FREQUENCY 140 // お使いのLRAの共振周波数(Hz)に合わせて調整
const unsigned long CYCLE_PERIOD_US = 1000000 / LRA_FREQUENCY; // 1サイクルの時間 (マイクロ秒)
const unsigned int PWM_STEPS = 20; // 1サイクルをいくつのステップで描画するか (解像度)
const unsigned int STEP_DELAY_US = CYCLE_PERIOD_US / PWM_STEPS; // 1ステップあたりの待機時間

/**
 * @brief DRV2605のレジスタに値を書き込む
 */
void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(DRV_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

/**
 * @brief DRV2605のレジスタから値を読み込む
 */
uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(DRV_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)DRV_ADDR, (uint8_t)1);
  return Wire.read();
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(250); // Power-up wait time
  Serial.println("DRV2605 Open-Loop Asymmetric Vibration Example");

  // --- DRV2605 Initialization ---
  writeReg(REG_MODE, 0x00); // Standbyモードを解除
  
  // ★ オープンループモードに設定 ★
  // Control3レジスタ (0x1D) の LRA_OPEN_LOOPビット (Bit 0) を1に設定
  uint8_t control3 = readReg(REG_CONTROL3);
  writeReg(REG_CONTROL3, control3 | 0x01); // Set LRA_OPEN_LOOP bit 
  
  writeReg(REG_MODE, 0x05); // RTPモードに設定
  Serial.println("Initialization complete. Open-Loop mode is active.");
}


void loop() {
  // --- パターンA: 正方向オフセット (正の時間が長い) ---
  Serial.println("--- Executing Positive-Offset Vibration ---");
  
  // 500ミリ秒間、パターンAの振動を続ける
  // ★ 修正点: 500を500Lにしてオーバーフローを防止
  for (int t = 0; t < (500L * 1000 / CYCLE_PERIOD_US); t++) {
    // 正の半サイクル (サイクルの75%の時間を使う)
    for (int i = 0; i < PWM_STEPS * 0.75; i++) {
      float angle = map(i, 0, (long)(PWM_STEPS * 0.75), 0, 180);
      int8_t amplitude = 127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(STEP_DELAY_US);
    }
    // 負の半サイクル (サイクルの25%の時間を使う)
    for (int i = 0; i < PWM_STEPS * 0.25; i++) {
      float angle = map(i, 0, (long)(PWM_STEPS * 0.25), 180, 360);
      int8_t amplitude = 127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(STEP_DELAY_US);
    }
  }
  writeReg(REG_RTP_INPUT, 0); // 停止
  delay(2000);


  // --- パターンB: 負方向オフセット (負の時間が長い) ---
  Serial.println("--- Executing Negative-Offset Vibration ---");

  // 500ミリ秒間、パターンBの振動を続ける
  // ★ 修正点: こちらも同様に500Lに修正
  for (int t = 0; t < (500L * 1000 / CYCLE_PERIOD_US); t++) {
    // 正の半サイクル (サイクルの25%の時間を使う)
    for (int i = 0; i < PWM_STEPS * 0.25; i++) {
      float angle = map(i, 0, (long)(PWM_STEPS * 0.25), 0, 180);
      int8_t amplitude = 127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(STEP_DELAY_US);
    }
    // 負の半サイクル (サイクルの75%の時間を使う)
    for (int i = 0; i < PWM_STEPS * 0.75; i++) {
      float angle = map(i, 0, (long)(PWM_STEPS * 0.75), 180, 360);
      int8_t amplitude = 127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(STEP_DELAY_US);
    }
  }
  writeReg(REG_RTP_INPUT, 0); // 停止
  delay(2000);
}