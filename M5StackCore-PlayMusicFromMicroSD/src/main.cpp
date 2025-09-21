// maou_se_system49
// asymmetrical_wave_44100Hz

#include <M5Unified.h>
#include <WiFi.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// --- グローバル変数 ---
AudioGeneratorWAV *wav = nullptr;
AudioFileSourceSD *file = nullptr;
AudioOutputI2S *out = nullptr; // 出力先は一度作ればOK

// 再生したいファイル名を指定
// const char* wav_path = "/maou_se_system49.wav";//魔王魂からとってきたwavファイル.再生できなかったので,フォーマットとかで大事な要素があるのかもしれない.
const char* wav_path = "/asymmetrical_wave_44100Hz.wav";

// --- 再生を開始（または再開）するための関数 ---
void startPlayback() {
  // 以前の再生インスタンスがあれば、メモリを解放する
  if (wav) {
    wav->stop();
    delete wav;
    wav = nullptr;
  }
  if (file) {
    delete file;
    file = nullptr;
  }

  M5.Lcd.println("Starting playback...");
  Serial.println("Starting playback...");

  // ファイルを新しく開き直す
  file = new AudioFileSourceSD(wav_path);
  if (!file->isOpen()) {
    M5.Lcd.println("Failed to re-open file!");
    return;
  }

  // WAV再生オブジェクトを新しく作成
  wav = new AudioGeneratorWAV();
  
  // 再生を開始
  wav->begin(file, out);
}


void setup()
{
  M5.begin();
  M5.Power.begin();
  WiFi.mode(WIFI_OFF); 
  delay(500);
  
  // --- SDカードの初期化 ---
  M5.Lcd.print("Initializing SD card...");
  if (!SD.begin(4, SPI, 24000000)) { 
    M5.Lcd.println("Initialization failed!");
    while (1);
  }
  M5.Lcd.println("OK!");

  // --- スピーカーの準備 ---
  M5.Speaker.begin();

  M5.Lcd.setTextFont(2);
  Serial.printf("Sample WAV playback begins...\n");
  
  // --- オーディオ出力のセットアップ (これは一度だけでOK) ---
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  out->SetGain(1.0); // 音量を100%に設定
  
  // --- 最初の再生を開始 ---
  startPlayback();

  // ファイルサイズの表示（デバッグ用）
  if (file) M5.Lcd.printf("File size : %d bytes\n", file->getSize());
}

void loop()
{
  // wavオブジェクトが存在し、かつ再生中かチェック
  if (wav && wav->isRunning()) {
    // wav->loop()がfalseを返したら、曲の終端に達したということ
    if (!wav->loop()) {
      Serial.println("Track ended. Restarting...");
      // 再生が終了したので、もう一度最初から再生する
      startPlayback();
    }
  }
}