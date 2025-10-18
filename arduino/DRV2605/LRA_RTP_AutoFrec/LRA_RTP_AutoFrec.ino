#include <Wire.h>

#define DRV_ADDR 0x5A

// --- DRV2605 Register Addresses ---
#define REG_MODE 0x01
#define REG_RTP_INPUT 0x02
#define REG_FEEDBACK_CONTROL 0x1A
#define REG_CONTROL3 0x1D
#define REG_LRA_PERIOD 0x22 // LRAの共振周期を読み出すレジスタ

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
  Serial.println("DRV2605 LRA Resonance Finder");

  // --- DRV2605 Initialization for Closed-Loop ---
  writeReg(REG_MODE, 0x00); // Standbyモードを解除
  
  // LRAモードを有効化 (N_ERM_LRAビットを1に)
  uint8_t feedback_control = readReg(REG_FEEDBACK_CONTROL);
  writeReg(REG_FEEDBACK_CONTROL, feedback_control | 0x80);
  
  // オープンループ・モードを「無効」に設定 (LRA_OPEN_LOOPビットを0に)
  uint8_t control3 = readReg(REG_CONTROL3);
  writeReg(REG_CONTROL3, control3 & 0xFE); 
  
  // RTP (リアルタイムプレイバック) モードに設定
  writeReg(REG_MODE, 0x05); 
  
  // 振動を開始 (これによりオートレゾナンス機能が働き始めます)
  writeReg(REG_RTP_INPUT, 127); // 100%の強さで振動開始
  
  Serial.println("Vibration started. Measuring resonance frequency...");
  Serial.println("---------------------------------------------");
}


void loop() {
  // レジスタからLRAの周期データを読み出す
  uint8_t period_reg_val = readReg(REG_LRA_PERIOD);

  //[cite_start]// データシートの式に基づいて周期(マイクロ秒)を計算 [cite: 1720]
  //[cite_start]// LRA period (us) = LRA_Period [7:0] x 98.46 us [cite: 1720]
  float period_us = period_reg_val * 98.46;

  // 周期から周波数(Hz)を計算
  // 周波数 = 1,000,000 / 周期(us)
  float frequency_hz = 1000000.0 / period_us;

  // 結果をシリアルモニタに表示
  Serial.print("Register[0x22]: ");
  Serial.print(period_reg_val);
  Serial.print("\t Measured Period: ");
  Serial.print(period_us);
  Serial.print(" us");
  Serial.print("\t Calculated Frequency: ");
  Serial.print(frequency_hz);
  Serial.println(" Hz");
  
  delay(2000); // 2秒ごとに測定・表示
}