#include <Wire.h>
#define DRV_ADDR 0x5A
#define REG_MODE 0x01
#define REG_RTP  0x02   // RTP input register（データシート要確認）

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(DRV_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void setup() {
  Wire.begin();
  delay(10);

  // RTPモードに設定（0x05がRTPモード）
  writeReg(REG_MODE, 0x05);
}

void loop() {
  // 片側を強め（200）、片側を弱め（100）
  uint8_t pattern_f[] = {
    200, 200, 200,   // 正側：強い
    100, 100,        // 負側：弱い
    127, 127         // ニュートラルに戻す
  };
    uint8_t pattern_b[] = {
    100, 100, 100,   // 正側：強い
    200, 200,        // 負側：弱い
    127, 127         // ニュートラルに戻す
  };

for (int j = 0; j < 10; j++) {
  for (int i = 0; i < sizeof(pattern_f); i++) {
    writeReg(REG_RTP, pattern_f[i]);
    delay(5); // 5msごとに更新 → 約200Hz
  }
}


  delay(1000);
for (int j = 0; j < 10; j++) {
    for (int i = 0; i < sizeof(pattern_b); i++) {
    writeReg(REG_RTP, pattern_b[i]);
    delay(5); // 5msごとに更新 → 約200Hz
  }
}
  

}

