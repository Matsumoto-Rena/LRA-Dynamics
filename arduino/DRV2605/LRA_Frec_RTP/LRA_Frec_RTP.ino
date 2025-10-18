#include <Wire.h>

#define DRV_ADDR 0x5A

// --- DRV2605 Register Addresses ---
#define REG_MODE 0x01
#define REG_RTP_INPUT 0x02
#define REG_CONTROL3 0x1D
#define REG_FEEDBACK_CONTROL 0x1A
#define REG_LRA_PERIOD 0x22

// --- LRA Parameters ---
float optimal_frequency = 175.0; // 測定した周波数を格納する変数 (初期値は仮)

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(DRV_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(DRV_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)DRV_ADDR, (uint8_t)1);
  return Wire.read();
}

void Positive_Offset(unsigned long cycle_period_us,unsigned int pwm_steps,unsigned int step_delay_us ) {
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

void Negative_Offset(unsigned long cycle_period_us,unsigned int pwm_steps,unsigned int step_delay_us ) {
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

void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(250);
  Serial.println("DRV2605 Auto-Tuned Asymmetric Vibration");


  // ★ ステップ1: クローズドループ・モードで最適な共振周波数を測定 ★
 
  Serial.println("--- Step 1: Measuring optimal frequency in Closed-Loop mode ---");
  writeReg(REG_MODE, 0x80); // デバイスリセット
  delay(10);
  
  writeReg(REG_MODE, 0x00); // Standby解除
  writeReg(REG_FEEDBACK_CONTROL, readReg(REG_FEEDBACK_CONTROL) | 0x80); // LRAモード有効化
  writeReg(REG_CONTROL3, readReg(REG_CONTROL3) & 0xFE); // オープンループを「無効」に
  
  writeReg(REG_MODE, 0x05); // RTPモードに設定
  writeReg(REG_RTP_INPUT, 127); // 100%の強さで振動開始
  delay(300); // オートレゾナンスが安定するまで待つ

  uint8_t period_reg_val = readReg(REG_LRA_PERIOD);
  float period_us = period_reg_val * 98.46;
  if (period_us > 0) {
    optimal_frequency = 1000000.0 / period_us;
  }
  
  writeReg(REG_RTP_INPUT, 0); // 測定が終わったので振動を停止
  delay(100);

  Serial.print("Optimal frequency measured: ");
  Serial.print(optimal_frequency);
  Serial.println(" Hz");
  Serial.println("----------------------------------------------------");
  //オープンループ・モードに切り替えて実行準備 ★
  writeReg(REG_MODE, 0x80); // デバイスリセット
  Serial.println("--- Step 2: Switching to Open-Loop mode for execution ---");
  writeReg(REG_CONTROL3, readReg(REG_CONTROL3) | 0x01); // オープンループを「有効」に
  writeReg(REG_MODE, 0x05); // 再度RTPモードに設定
  Serial.println("Initialization complete. Starting asymmetric vibration loop.");
}


void loop() {

  unsigned long cycle_period_us = 1000000 / optimal_frequency;
  unsigned int pwm_steps = 20;
  unsigned int step_delay_us = cycle_period_us / pwm_steps;
  Positive_Offset(cycle_period_us,pwm_steps,step_delay_us);
  Negative_Offset(cycle_period_us,pwm_steps,step_delay_us);


}