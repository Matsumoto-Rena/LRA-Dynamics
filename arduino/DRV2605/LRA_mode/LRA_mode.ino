#include <Wire.h>
#include "Adafruit_DRV2605.h"

Adafruit_DRV2605 drv;

void setup() {
  Serial.begin(115200);
  Serial.println("DRV2605L test for LRA");

  if (!drv.begin()) {
    Serial.println("Could not find DRV2605");
    while (1);
  }

  // LRAモードに設定
  drv.setMode(DRV2605_MODE_INTTRIG); // 内部トリガーモード
  drv.useLRA();
}

void loop() {
  // 効果番号を選ぶ（TIが定義したハプティックエフェクト）
  drv.setWaveform(0, 1);   // エフェクト #1
  drv.setWaveform(1, 0);   // 終端コード 0

  // 再生
  drv.go();

  delay(1000);
}
