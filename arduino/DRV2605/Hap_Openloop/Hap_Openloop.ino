#include <Wire.h>

#define DRV_ADDR 0x5A
// --- Register Addresses ---
#define REG_MODE 0x01
#define REG_RTP_INPUT 0x02
#define REG_CONTROL3 0x1D

// writeReg, readReg関数は変更なし
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

void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(250);
  Serial.println("Haptic Reactor 4-Axis Optimized Movement");
  // DRV2605 Initialization (Open-Loop)
  writeReg(REG_MODE, 0x80);
  delay(10);
  writeReg(REG_MODE, 0x00);
  writeReg(REG_CONTROL3, readReg(REG_CONTROL3) | 0x01);
  writeReg(REG_MODE, 0x05);
  Serial.println("Initialization complete.");
}

/**
 * @brief FORWARD/RIGHT用の波形 (ゆっくり押して、素早く引く)
 * 非対称サイン波 (正の半サイクルが長い)
 */
void generateForwardWave(float frequency, int duration_ms) {
  const unsigned long CYCLE_PERIOD_US = 1000000 / frequency;
  const unsigned int PWM_STEPS = 20;
  const unsigned int STEP_DELAY_US = CYCLE_PERIOD_US / PWM_STEPS;
  long num_cycles = ((long)duration_ms * 1000) / CYCLE_PERIOD_US;

  for (long t = 0; t < num_cycles; t++) {
    // 正の半サイクル (75%の時間)
    for (int i = 0; i < (int)(PWM_STEPS * 0.75); i++) {
      float angle = map(i, 0, (long)(PWM_STEPS * 0.75), 0, 180);
      writeReg(REG_RTP_INPUT, (uint8_t)(127 * sin(radians(angle))));
      delayMicroseconds(STEP_DELAY_US);
    }
    // 負の半サイクル (25%の時間)
    for (int i = 0; i < (int)(PWM_STEPS * 0.25); i++) {
      float angle = map(i, 0, (long)(PWM_STEPS * 0.25), 180, 360);
      writeReg(REG_RTP_INPUT, (uint8_t)(int8_t)(127 * sin(radians(angle))));
      delayMicroseconds(STEP_DELAY_US);
    }
  }
  writeReg(REG_RTP_INPUT, 0);
}

/**
 * @brief ★改善案★ BACKWARD/LEFT用の波形 (ゆっくり引いて、素早く戻す)
 */
void generateBackwardWave(float frequency, int duration_ms) {
  const unsigned long CYCLE_PERIOD_US = 1000000 / frequency;
  const unsigned int PWM_STEPS = 20;
  const unsigned int STEP_DELAY_US = CYCLE_PERIOD_US / PWM_STEPS;
  long num_cycles = ((long)duration_ms * 1000) / CYCLE_PERIOD_US;

  for (long t = 0; t < num_cycles; t++) {
    // 負の半サイクル (75%の時間)
    for (int i = 0; i < (int)(PWM_STEPS * 0.75); i++) {
      float angle = map(i, 0, (long)(PWM_STEPS * 0.75), 180, 360);
      writeReg(REG_RTP_INPUT, (uint8_t)(int8_t)(127 * sin(radians(angle))));
      delayMicroseconds(STEP_DELAY_US);
    }
    // 正の半サイクル (25%の時間)
    for (int i = 0; i < (int)(PWM_STEPS * 0.25); i++) {
      float angle = map(i, 0, (long)(PWM_STEPS * 0.25), 0, 180);
      writeReg(REG_RTP_INPUT, (uint8_t)(127 * sin(radians(angle))));
      delayMicroseconds(STEP_DELAY_US);
    }
  }
  writeReg(REG_RTP_INPUT, 0);
}

void generateSineWave_160_F(float frequency, int duration_ms) {
  const unsigned long CYCLE_PERIOD_US = 1000000 / frequency;
  const unsigned int PWM_STEPS = 20;
  const unsigned int STEP_DELAY_US = CYCLE_PERIOD_US / PWM_STEPS;
  long num_cycles = ((long)duration_ms * 1000) / CYCLE_PERIOD_US;

  for (long t = 0; t < num_cycles; t++) {
    for (int i = 0; i < PWM_STEPS; i++) {
      float angle = map(i, 0, PWM_STEPS, 0, 360);
      int8_t amplitude = 127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(STEP_DELAY_US);
    }
    
  }

  
  writeReg(REG_RTP_INPUT, 0); // 停止
}

void generateSineWave_160_B(float frequency, int duration_ms) {
  const unsigned long CYCLE_PERIOD_US = 1000000 / frequency;
  const unsigned int PWM_STEPS = 20;
  const unsigned int STEP_DELAY_US = CYCLE_PERIOD_US / PWM_STEPS;
  long num_cycles = ((long)duration_ms * 1000) / CYCLE_PERIOD_US;

  for (long t = 0; t < num_cycles; t++) {
    for (int i = 0; i < PWM_STEPS; i++) {
      float angle = map(i, 0, PWM_STEPS, 0, 180);
      int8_t amplitude = -127 * sin(radians(angle));
      writeReg(REG_RTP_INPUT, (uint8_t)amplitude);
      delayMicroseconds(STEP_DELAY_US);
    }
  }
  writeReg(REG_RTP_INPUT, 0); // 停止
}

void loop() {
  const int DURATION_MS = 1500;

  // ★★★ 160Hz軸 (前後) の最適周波数を設定 ★★★
  // (160-165Hzの間で最も強かった値に調整してください)
  float freq_axis1 = 170.0; // 例: 163Hz

  // 320Hz軸 (左右) の周波数
  float freq_axis2 = 320.0;

  Serial.println("--- 1. Moving FORWARD ---");
  generateSineWave_160_F(freq_axis1, 2500);// 160Hz軸で正オフセット
  delay(2000);

  Serial.println("--- 2. Moving BACKWARD ---");
  generateSineWave_160_B(freq_axis1, 2500); // 160Hz軸で負オフセット
  delay(2000);

  Serial.println("--- 3. Moving RIGHT ---");
  generateForwardWave(freq_axis2,DURATION_MS); // 320Hz軸で正オフセット
  delay(2000);

  Serial.println("--- 4. Moving LEFT ---");
  generateBackwardWave(freq_axis2,DURATION_MS); // 320Hz軸で負オフセット
  delay(2000);
}