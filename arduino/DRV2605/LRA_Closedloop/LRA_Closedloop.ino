#include <Wire.h>

#define DRV_ADDR 0x5A

// --- DRV2605 Register Addresses ---
#define REG_MODE 0x01
#define REG_RTP_INPUT 0x02
#define REG_CONTROL3 0x1D
#define REG_FEEDBACK_CONTROL 0x1A

/**
 * @brief DRV2605のレジスタに値を書き込む
 */
void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(DRV_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void Positive_Offset_sin(unsigned long cycle_period_us,unsigned int pwm_steps,unsigned int step_delay_us ) {
    // ステップ3で測定した周波数を元に各種パラメータを計算


  // --- パターンA: 正方向オフセット (正の時間が長い) ---
  for (int t = 0; t < (500L * 1000 / cycle_period_us); t++) {
    for (int i = 0; i < pwm_steps * 0.75; i++) {
      float angle = map(i, 0, (long)(pwm_steps * 0.75), 0, 180);
      int8_t amplitude = 127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(step_delay_us);
    }
    for (int i = 0; i < pwm_steps * 0.25; i++) {
      float angle = map(i, 0, (long)(pwm_steps * 0.25), 180, 360);
      int8_t amplitude = 127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(step_delay_us);
    }
  }
  writeReg(REG_RTP_INPUT, 0);
  delay(2000);
}

void Positive_Offset(unsigned long cycle_period_us,unsigned int pwm_steps,unsigned int step_delay_us ) {
    // ステップ3で測定した周波数を元に各種パラメータを計算


  // --- パターンA: 正方向オフセット (正の時間が長い) ---
  for (int t = 0; t < (500L * 1000 / cycle_period_us); t++) {
    for (int i = 0; i < pwm_steps * 0.75; i++) {
      writeReg(REG_RTP_INPUT, 127);
      delayMicroseconds(step_delay_us);
    }
    for (int i = 0; i < pwm_steps * 0.25; i++) {
      writeReg(REG_RTP_INPUT, -127);
      delayMicroseconds(step_delay_us);
    }
  }
  writeReg(REG_RTP_INPUT, 0);
  delay(2000);
}

void Negative_Offset_sin(unsigned long cycle_period_us,unsigned int pwm_steps,unsigned int step_delay_us ) {
    // --- パターンB: 負方向オフセット (負の時間が長い) ---
  for (int t = 0; t < (500L * 1000 / cycle_period_us); t++) {
    for (int i = 0; i < pwm_steps * 0.25; i++) {
      float angle = map(i, 0, (long)(pwm_steps * 0.25), 0, 180);
      int8_t amplitude = 127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(step_delay_us);
    }
    for (int i = 0; i < pwm_steps * 0.75; i++) {
      float angle = map(i, 0, (long)(pwm_steps * 0.75), 180, 360);
      int8_t amplitude = 127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(step_delay_us);
    }
  }
  writeReg(REG_RTP_INPUT, 0);
  delay(2000);
}

void Negative_Offset(unsigned long cycle_period_us,unsigned int pwm_steps,unsigned int step_delay_us ) {
    // ステップ3で測定した周波数を元に各種パラメータを計算


  // --- パターンA: 正方向オフセット (正の時間が長い) ---
  for (int t = 0; t < (500L * 1000 / cycle_period_us); t++) {
    for (int i = 0; i < pwm_steps * 0.75; i++) {
      writeReg(REG_RTP_INPUT, -127);
      delayMicroseconds(step_delay_us);
    }
    for (int i = 0; i < pwm_steps * 0.25; i++) {
      writeReg(REG_RTP_INPUT, 127);
      delayMicroseconds(step_delay_us);
    }
  }
  writeReg(REG_RTP_INPUT, 0);
  delay(2000);
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
float optimal_frequency = 120;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(250); // Power-up wait time
  Serial.println("DRV2605 Closed-Loop Asymmetric Vibration Example");
  writeReg(REG_MODE, 0x80);
  // --- DRV2605 Initialization ---
  writeReg(REG_MODE, 0x00); // Standbyモードを解除
  
  // ★ クローズドループモードに設定（デフォルト） ★
  // LRAモードを有効化し、オープンループは無効のままにする
  uint8_t feedback_control = readReg(REG_FEEDBACK_CONTROL);
  writeReg(REG_FEEDBACK_CONTROL, feedback_control | 0x80); // LRAモードを有効化

  uint8_t control3 = readReg(REG_CONTROL3);
  writeReg(REG_CONTROL3, readReg(REG_CONTROL3) & 0xFE); // LRA_OPEN_LOOPビットを0にクリア
  
  writeReg(REG_MODE, 0x05); // RTPモードに設定
  Serial.println("Initialization complete. Closed-Loop mode is active.");
}


void loop() {

  unsigned long cycle_period_us = 1000000 / optimal_frequency;
  unsigned int pwm_steps = 20;
  unsigned int step_delay_us = cycle_period_us / pwm_steps;
  // --- パターンA: 正方向オフセット (ゆっくり加速 -> 急停止) ---
  Serial.println("--- Executing Positive-Offset Vibration (Ramp Up) ---");

  // forループで振幅を徐々に上げていく (DRVが最適な周波数で駆動)

  Positive_Offset(cycle_period_us,pwm_steps,step_delay_us);
  // // 完全に停止
  // writeReg(REG_RTP_INPUT, 0);
  delay(2000); // 2秒待機


  // --- パターンB: 負方向オフセット (急加速 -> ゆっくり停止) ---
  Serial.println("--- Executing Negative-Offset Vibration (Ramp Down) ---");
  writeReg(REG_RTP_INPUT, -127);
  Negative_Offset(cycle_period_us,pwm_steps,step_delay_us);


  // // 完全に停止
  // writeReg(REG_RTP_INPUT, 0);
  delay(2000); // 2秒待機
}