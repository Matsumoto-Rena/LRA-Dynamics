#include <Wire.h>

#define DRV_ADDR 0x5A

// --- DRV2605 Register Addresses ---
#define REG_MODE              0x01
#define REG_RTP_INPUT         0x02
#define REG_LIBRARY_SEL       0x03
#define REG_GO                0x0C
#define REG_RATED_VOLTAGE     0x16
#define REG_OD_CLAMP          0x17
#define REG_FEEDBACK_CONTROL  0x1A
#define REG_CONTROL1          0x1B
#define REG_CONTROL2          0x1C
#define REG_CONTROL3          0x1D
#define REG_STATUS            0x00

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

void moveForward() {
  Serial.println("--- Executing Forward Pattern (Ramp Up -> Sharp Stop) ---");
  // 500ミリ秒間、このパターンを繰り返す
  for (int t = 0; t < 2; t++) { // 繰り返し回数で時間を調整
    // 振幅を0から127まで徐々に上げていく（ゆっくり加速）
    for (int i = 0; i <= 127; i++) {
      writeReg(REG_RTP_INPUT, i);
      delay(2); // 1サイクル約250ms
    }
    // 強いブレーキで急停止
    writeReg(REG_RTP_INPUT, (uint8_t)-127);
    delay(50);
  }
  writeReg(REG_RTP_INPUT, 0); // 完全に停止
}

/**
 * @brief 後進パターン（急加速 -> ゆっくり停止）を実行
 */
void moveBackward() {
  Serial.println("--- Executing Backward Pattern (Sharp Pulse -> Ramp Down) ---");
  // 500ミリ秒間、このパターンを繰り返す
  for (int t = 0; t < 2; t++) { // 繰り返し回数で時間を調整
    // 最初に強い振幅を瞬間的に与える（急加速）
    writeReg(REG_RTP_INPUT, 127);
    delay(50);
    // 振幅を127から0まで徐々に下げていく（ゆっくり停止）
    for (int i = 127; i >= 0; i--) {
      writeReg(REG_RTP_INPUT, i);
      delay(2); // 1サイクル約250ms
    }
  }
  writeReg(REG_RTP_INPUT, 0); // 完全に停止
}

/**
 * @brief 後進パターン（急加速 -> 自然減衰）
 */
void moveBackward_v2() {
  Serial.println("--- Executing Backward Pattern (Sharp Pulse -> Natural Decay) ---");
  
  // 非常に短い、強力なパルスを与える
  writeReg(REG_RTP_INPUT, 127);
  delay(30); // パルス幅 (20ms ~ 50msで調整)
  
  // その後、即座に駆動を停止し、自然に減衰させる
  writeReg(REG_RTP_INPUT, 0);
  
  // パターン全体が1サイクルになるように待機
  delay(250); // moveForwardの1サイクル時間と合わせる
}

/**
 * @brief 後進パターン（ダブルクリックによる鋭いキック）
 */
void moveBackward_v3() {
  Serial.println("--- Executing Backward Pattern (Double-Click Kick) ---");
  
  for (int t = 0; t < 4; t++) { // 繰り返し回数で時間を調整
    // 1. 非常に短い、強力な正パルス
    writeReg(REG_RTP_INPUT, 127);
    delay(20); // パルス幅 (短くするほど鋭くなる)
    
    // 2. 直後に強力なブレーキをかける
    writeReg(REG_RTP_INPUT, (uint8_t)-127);
    delay(40); // ブレーキ時間
    
    // 3. 次のキックまでのインターバル
    writeReg(REG_RTP_INPUT, 0);
    delay(80); // 1サイクルが約140msになるように調整
  }
  
  writeReg(REG_RTP_INPUT, 0); // 完全に停止
}
/**
 * @brief DRV2605のレジスタから値を読み込む
 */

float optimal_frequency = 120;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(250);
  Serial.println("DRV2605 Closed-Loop with Auto-Calibration");

  // --- 1. デバイスリセット ---
  writeReg(REG_MODE, 0x80);
  delay(10);

  // --- 2. キャリブレーションの準備 ---
  Serial.println("Preparing for auto-calibration...");
  writeReg(REG_MODE, 0x00); // Standby解除

  // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
  // ★重要：お使いのLRAのデータシートに合わせて以下の値を設定してください★
  // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
  // 例: 定格電圧(Rated Voltage)が1.8VrmsのLRAの場合
  // Vrms = 20.71e-3 * RATED_VOLTAGE / sqrt(1 - (4*t_sample + 300e-6)*f)
  // 簡単な計算では RATED_VOLTAGE ≒ Vrms * 48 程度。 1.8V * 48 ≒ 86 (0x56)
  writeReg(REG_RATED_VOLTAGE, 0x56); // 例: 1.8Vrms LRA

  // 例: オーバードライブ電圧(Overdrive Clamp)を3.0Vpkに設定
  // Vpk = 21.96e-3 * OD_CLAMP。 OD_CLAMP ≒ Vpk * 45.5 程度。 3.0V * 45.5 ≒ 136 (0x88)
  writeReg(REG_OD_CLAMP, 0x88); // 例: 3.0Vpk

  // フィードバック制御とLRAモードを設定
  writeReg(REG_FEEDBACK_CONTROL, 0xB6); // LRAモード, Brake Factor=3x, Loop Gain=Medium, BEMF Gain=20x
  writeReg(REG_CONTROL1, 0x13);
  writeReg(REG_CONTROL2, 0xF5);
  writeReg(REG_CONTROL3, 0x80);

  // --- 3. キャリブレーションの実行 ---
  Serial.println("Running auto-calibration...");
  writeReg(REG_MODE, 0x07); // 自動キャリブレーションモードに設定
  writeReg(REG_GO, 0x01);   // キャリブレーション開始

  // GOビットが自動で0に戻るまで待つ
  while (readReg(REG_GO) != 0) {
    delay(10);
  }
  Serial.println("Calibration finished.");

  // --- 4. 結果の確認 ---
  uint8_t status = readReg(REG_STATUS);
  if ((status & 0b00001000) != 0) { // DIAG_RESULTビットを確認
    Serial.println("!!! Calibration FAILED. Check wiring and LRA specs. !!!");
    while(1); // 失敗したら停止
  } else {
    Serial.println(">>> Calibration SUCCESS. <<<");
  }

  // --- 5. 通常の振動モードに設定 ---
  writeReg(REG_MODE, 0x05); // RTPモードに設定
  Serial.println("Switched to RTP mode. Starting continuous vibration.");
}


void loop() {

  unsigned long cycle_period_us = 1000000 / optimal_frequency;
  unsigned int pwm_steps = 20;
  unsigned int step_delay_us = cycle_period_us / pwm_steps;
  // --- パターンA: 正方向オフセット (ゆっくり加速 -> 急停止) ---

  // forループで振幅を徐々に上げていく (DRVが最適な周波数で駆動)

  Positive_Offset_sin(cycle_period_us,pwm_steps,step_delay_us);
  // // 完全に停止
  // writeReg(REG_RTP_INPUT, 0);
  // moveForward();
  delay(2000); // 2秒待機


  // --- パターンB: 負方向オフセット (急加速 -> ゆっくり停止) ---
  Negative_Offset_sin(cycle_period_us,pwm_steps,step_delay_us);


  // // 完全に停止
  // writeReg(REG_RTP_INPUT, 0);
  // moveBackward_v2();
  delay(2000); // 2秒待機
}