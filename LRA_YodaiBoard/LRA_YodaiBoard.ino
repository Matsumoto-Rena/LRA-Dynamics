#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac1;

// 16段階 (半波 8段階) のテーブル
const PROGMEM uint16_t DACLookup_FullSine_5Bit[16] =
{
  2048, 2831, 3495, 3939,
  4095, 3939, 3495, 2831,
  2048, 1264,  600,  156,
     0,  156,  600, 1264
};

// ----- 2種類の周波数を定義 -----
// 160Hz (1/160秒) の 1サンプル時間 (781 us)
const long SAMPLE_PERIOD_160HZ = 1000000L / (160.0 * 8L);
// 320Hz (1/320秒) の 1サンプル時間 (390 us)
const long SAMPLE_PERIOD_320HZ = 1000000L / (320.0 * 8L);
// ------------------------------

// ★ 現在のサンプリング周期 (最初は 160Hz 用)
unsigned long currentSamplePeriod = SAMPLE_PERIOD_160HZ;

unsigned long nextSampleTime = 0;
uint8_t sampleIndex = 0; // 0-7
uint8_t cycleCount = 0;  // 0-3 (正, 正, 正, 負)

// --- パターン＆状態管理用の変数 ---
enum State {
  RUNNING,
  WAITING
};
State currentState = RUNNING; // 初期状態は再生中

bool isPatternA = true;     // 現在がパターンA (正正正負) か

const uint16_t patternRepeats = 100; // 100回
const uint16_t switchWaveCount = patternRepeats * 4; // 400波形
uint16_t halfWaveCounter = 0;  // 出力した半波の数をカウント

unsigned long waitStartTime = 0; // 待機を開始した時間 (millis)
const long waitDuration = 1000;  // 1秒待機
// ----------------------------------

void setup() {
  Serial.begin(115200); 
  dac1.begin(0x62);
  // Wire.setClock() は不要

  Serial.println("--- 160Hzモード スタート ---");
  Serial.print("Sample Period: ");
  Serial.print(currentSamplePeriod); // 781 us
  Serial.println(" us");

  // タイマーを初期化
  nextSampleTime = micros(); 
  Serial.println(">> Pattern A: 正正正負 (100回)");
}

void loop() {
  
  if (currentState == RUNNING) {
    // --- (状態1) 波形を再生中 ---
    
    if (micros() >= nextSampleTime) {
      
      uint16_t dacValue;
      uint8_t readIndex; 

      if (isPatternA) {
        // Pattern A: 正正正負
        (cycleCount < 3) ? readIndex = sampleIndex : readIndex = sampleIndex + 8;
      } else {
        // Pattern B: 負負負正
        (cycleCount < 3) ? readIndex = sampleIndex + 8 : readIndex = sampleIndex;
      }
      
      dacValue = pgm_read_word(&(DACLookup_FullSine_5Bit[readIndex]));
      dac1.setVoltage(dacValue, false, 800000);
      
      sampleIndex = (sampleIndex + 1) % 8;
      
      if (sampleIndex == 0) {
        cycleCount = (cycleCount + 1) % 4;
        halfWaveCounter++;

        if (halfWaveCounter >= switchWaveCount) {
          currentState = WAITING;
          waitStartTime = millis();
          halfWaveCounter = 0;
          dac1.setVoltage(2048, false, 800000);
          // Serial.println(">> 1秒待機中...");
        }
      }
      
      // ★ 変数になったサンプリング周期を使う
      nextSampleTime += currentSamplePeriod; 
    }
    
  } else {
    // --- (状態2) 1秒待機中 ---
    
    if (millis() - waitStartTime >= waitDuration) {
      
      isPatternA = !isPatternA; // パターンを反転
      currentState = RUNNING;
      cycleCount = 0;
      nextSampleTime = micros(); 

      // ★ パターンAに戻るタイミング (負負負正が終わった後) かどうか
      if (isPatternA == true) {
        
        // ★ 周波数を切り替える
        if (currentSamplePeriod == SAMPLE_PERIOD_160HZ) {
          // 160Hz -> 320Hz へ
          currentSamplePeriod = SAMPLE_PERIOD_320HZ;
          // Serial.println("--- 320Hzモード スタート ---");
        } else {
          // 320Hz -> 160Hz へ
          currentSamplePeriod = SAMPLE_PERIOD_160HZ;
          // Serial.println("--- 160Hzモード スタート ---");
        }
        
        // Serial.print("Sample Period: ");
        // Serial.print(currentSamplePeriod);
        // Serial.println(" us");
        // Serial.println(">> Pattern A: 正正正負 (100回)");

      } else {
        // (パターンBが始まるだけなら周波数は変えない)
        // Serial.println(">> Pattern B: 負負負正 (100回)");
      }
    }
  }
}