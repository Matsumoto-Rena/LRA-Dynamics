#include <Wire.h>

#define DRV_ADDR 0x5A

// --- DRV2605 Register Addresses ---
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
  Serial.println("Haptic Reactor Frequency Sweep Test");

  // --- DRV2605 Initialization (Open-Loop) ---
  writeReg(REG_MODE, 0x80); // デバイスリセット
  delay(10);
  writeReg(REG_MODE, 0x00); // Standbyモードを解除
  
  // オープンループモードに設定
  uint8_t control3 = readReg(REG_CONTROL3);
  writeReg(REG_CONTROL3, control3 | 0x01);
  
  writeReg(REG_MODE, 0x05); // RTPモードに設定
  Serial.println("Initialization complete. Starting sweep...");
}

/**
 * @brief 指定された周波数のサイン波を生成する関数
 * @param frequency 駆動周波数 (Hz)
 * @param duration_ms 振動を続ける時間 (ms)
 */
void generateSineWave(float frequency, int duration_ms) {
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

void loop() {
  const int DURATION_MS = 1500; // 各周波数での振動時間 (ms)

  // --- 160Hz周辺の周波数をスイープ ---
  Serial.println("\n--- Sweeping around 160Hz (140Hz to 180Hz) ---");
  delay(2000);
  for (int freq = 140; freq <= 180; freq++) {
    Serial.print("Testing: ");
    Serial.print(freq);
    Serial.println(" Hz");
    generateSineWave((float)freq, DURATION_MS);
    delay(500); // 次の周波数までのインターバル
  }

  Serial.println("\n--- First sweep finished. Pausing for 5 seconds. ---");
  delay(5000);

  // --- 320Hz周辺の周波数をスイープ ---
  Serial.println("\n--- Sweeping around 320Hz (300Hz to 340Hz) ---");
  delay(2000);
  for (int freq = 300; freq <= 340; freq++) {
    Serial.print("Testing: ");
    Serial.print(freq);
    Serial.println(" Hz");
    generateSineWave((float)freq, DURATION_MS);
    delay(500); // 次の周波数までのインターバル
  }

  Serial.println("\n--- All sweeps finished. Restarting in 10 seconds. ---");
  delay(10000);
}