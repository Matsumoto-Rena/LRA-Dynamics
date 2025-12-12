#include <Adafruit_MCP4725.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "" your wifi ssid
const char* password = "" your wifi password 

Adafruit_MCP4725 dac1;
WebServer server(80);

// --- 状態管理 ---
bool isAutoRunning = false; // 自動ループ中か
int manualId = 0;           // 手動実行中のパターンID (0=停止, 1~4=実行中)

// --- パラメータ変数 ---
int paramRes1 = 5;
int paramRep1 = 4;
int paramRes2 = 10;
int paramRep2 = 4;
int loopCount = 200;

void stopAll() {
  digitalWrite(D2, LOW); digitalWrite(D3, LOW);
  digitalWrite(D6, LOW); digitalWrite(D7, LOW);
  digitalWrite(D8, LOW); digitalWrite(D10, LOW);
  dac1.setVoltage(0, false, 800000);
}

// 停止指令が来ていないかチェックする関数
// 手動モードで指を離した(manualIdが0になった)ら true を返す
bool checkStop() {
  server.handleClient(); // 通信を処理
  // 手動モード実行中なのに、通信によって停止(0)に書き換わっていたら中断
  if (manualId == 0 && !isAutoRunning) return true; 
  
  // 自動モード中にSTOPされたら中断
  if (isAutoRunning == false && manualId == 0) return true;

  return false;
}

void move(int res, int repeatCount, bool isDir){
  if(res <= 0) res = 1;
  if(repeatCount < 0) repeatCount = 0;

  // delayMicroseconds(100);
  if(checkStop()) return; // ループの途中でも指が離れたら即終了
  for(int i = 0; i < repeatCount; i++){

    if(isDir){
      digitalWrite(D2,LOW); digitalWrite(D3,HIGH);
    }else{
      digitalWrite(D2,HIGH); digitalWrite(D3,LOW);
    }
    for(uint8_t k=0; k<res; k++){
      dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
    }
  }

  // delayMicroseconds(100);
  if(checkStop()) return;

  if(isDir){
    digitalWrite(D2,HIGH); digitalWrite(D3,LOW);
  }else{
    digitalWrite(D2,LOW); digitalWrite(D3,HIGH);
  }
  for(uint8_t k=0; k<res; k++){
    dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
  }
}

void move2(int res, int repeatCount, bool isDir){
  if(res <= 0) res = 1;
  if(repeatCount < 0) repeatCount = 0;

  delayMicroseconds(100);
  for(int i = 0; i < repeatCount; i++){
  if(checkStop()) return; // ループの途中でも指が離れたら即終了

    if(isDir){
      digitalWrite(D2,LOW); digitalWrite(D3,HIGH);
    }else{
      digitalWrite(D2,HIGH); digitalWrite(D3,LOW);
    }
    for(uint8_t k=0; k<res; k++){
      dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
    }
  }

  delayMicroseconds(100);
  if(checkStop()) return;
  
  if(isDir){
    digitalWrite(D2,HIGH); digitalWrite(D3,LOW);
  }else{
    digitalWrite(D2,LOW); digitalWrite(D3,HIGH);
  }
  for(uint8_t k=0; k<res; k++){
    dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
  }
}

// 指定したパターンの設定で1単位だけ動かす
void runPatternStep(int id) {
  // ピン設定
  digitalWrite(D6, LOW); digitalWrite(D8, LOW); digitalWrite(D7, LOW); digitalWrite(D10, LOW);
  
  switch(id) {
    case 1: digitalWrite(D6, HIGH);  for(uint8_t i=0; i < 10; i++) move(paramRes1, paramRep1, true);  break;
    case 2: digitalWrite(D8, HIGH);  for(uint8_t i=0; i < 10; i++) move(paramRes1, paramRep1, false); break;
    case 3: digitalWrite(D7, HIGH);  move2(paramRes2, paramRep2, true);  break;
    case 4: digitalWrite(D10, HIGH); move2(paramRes2, paramRep2, false); break;
  }
}

void handleRoot() {
  String html = "<html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>LRA Hold Control</title>";
  html += "<style>";
  html += "body{font-family:sans-serif; padding:10px; text-align:center; user-select:none; -webkit-user-select:none;}"; // 長押しでテキスト選択されないように
  html += "input[type='number']{padding:5px; width:50px;}";
  html += ".btn{width:45%; height:50px; font-size:16px; margin:5px; border:none; color:white; border-radius:5px;}";
  html += ".start{background-color:#28a745;} .stop{background-color:#dc3545;} .update{background-color:#007bff; width:95%;}";
  
  // 押している間だけ用のボタンスタイル
  html += ".hold-btn{background-color:#6c757d; height:100px; font-weight:bold; touch-action: none;}"; // touch-action:noneでスクロール防止
  html += ".hold-btn:active{background-color:#ffc107; color:black; transform: scale(0.98);}"; // 押している時の色
  html += "</style>";
  
  // JavaScript: 押した時と離した時の処理
  html += "<script>";
  html += "function start(id) { fetch('/manual_start?id=' + id); }";
  html += "function end() { fetch('/manual_stop'); }";
  
  // スマホとPC両方のイベントを登録するヘルパー
  html += "function reg(id) {";
  html += "  let b = document.getElementById('b'+id);";
  html += "  b.addEventListener('mousedown', function(e){ start(id); });";
  html += "  b.addEventListener('mouseup',   function(e){ end(); });";
  html += "  b.addEventListener('mouseleave',function(e){ end(); });"; // ボタンからカーソルが外れた時も停止
  html += "  b.addEventListener('touchstart',function(e){ e.preventDefault(); start(id); });";
  html += "  b.addEventListener('touchend',  function(e){ e.preventDefault(); end(); });";
  html += "}";
  html += "window.onload = function(){ reg(1); reg(2); reg(3); reg(4); };";
  html += "</script>";
  
  html += "</head><body>";
  html += "<h2>HOLD to Vibrate</h2>";
  
  // 手動ボタン (IDを付与)
  html += "<div>";
  html += "<button id='b1' class='btn hold-btn'>A (D6)<br>Res1/Rep1</button>";
  html += "<button id='b2' class='btn hold-btn'>B (D8)<br>Res1/Rep1</button>";
  html += "</div>";
  html += "<div>";
  html += "<button id='b3' class='btn hold-btn'>C (D7)<br>Res2/Rep2</button>";
  html += "<button id='b4' class='btn hold-btn'>D (D10)<br>Res2/Rep2</button>";
  html += "</div>";

  html += "<hr>";
  
  // 自動ループ
  html += "<h3>Auto Loop</h3>";
  html += "<button class='btn start' onclick=\"location.href='/start'\">START LOOP</button>";
  html += "<button class='btn stop' onclick=\"location.href='/stop'\">STOP LOOP</button>";

  html += "<hr>";
  html += "<form action='/set' method='GET'>";
  html += "<b>Pattern 1 (A/B)</b><br>";
  html += "Res: <input type='number' name='res1' value='" + String(paramRes1) + "'> ";
  html += "Rep: <input type='number' name='rep1' value='" + String(paramRep1) + "'><br><br>";
  
  html += "<b>Pattern 2 (C/D)</b><br>";
  html += "Res: <input type='number' name='res2' value='" + String(paramRes2) + "'> ";
  html += "Rep: <input type='number' name='rep2' value='" + String(paramRep2) + "'><br><br>";
  html += "<b>Auto Loop</b><br>Count: <input type='number' name='loops' value='" + String(loopCount) + "'><br><br>";
  html += "<input type='submit' value='UPDATE SETTINGS' class='btn update'></form>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("res1")) paramRes1 = server.arg("res1").toInt();
  if (server.hasArg("rep1")) paramRep1 = server.arg("rep1").toInt();
  if (server.hasArg("res2")) paramRes2 = server.arg("res2").toInt();
  if (server.hasArg("rep2")) paramRep2 = server.arg("rep2").toInt();
  if (server.hasArg("loops")) loopCount = server.arg("loops").toInt();
  server.sendHeader("Location", "/");
  server.send(303);
}

// 手動開始
void handleManualStart() {
  if (server.hasArg("id")) {
    isAutoRunning = false; // 自動モードはキャンセル
    manualId = server.arg("id").toInt();
  }
  server.send(200, "text/plain", "START OK");
}

// 手動停止
void handleManualStop() {
  manualId = 0; // IDを0にするとloop内で停止処理が走る
  stopAll();
  server.send(200, "text/plain", "STOP OK");
}

void handleStart() { isAutoRunning = true; manualId = 0; server.sendHeader("Location", "/"); server.send(303); }
void handleStop() { isAutoRunning = false; manualId = 0; stopAll(); server.sendHeader("Location", "/"); server.send(303); }

void setup() {
  Serial.begin(115200);

  pinMode(D2,OUTPUT); pinMode(D3,OUTPUT);
  pinMode(D6,OUTPUT); pinMode(D7,OUTPUT);
  pinMode(D8,OUTPUT); pinMode(D10,OUTPUT);

  dac1.begin(0x62);
  stopAll();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  Serial.println("\nIP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  
  // 手動操作用のAPI
  server.on("/manual_start", handleManualStart);
  server.on("/manual_stop", handleManualStop);
  
  server.begin();
}

void loop() {
  server.handleClient();

  // --- 手動モード実行中 (manualIdが1~4の間) ---
  if (manualId > 0) {
    runPatternStep(manualId);
    // runPatternStepの中でcheckStop()しているので、指を離せば次回ループでmanualId=0になり止まる
    return; 
  }

  // --- 自動ループモード ---
  if (isAutoRunning) {
    // 順番に実行。各ステップで停止チェック
    server.handleClient();
    runPatternStep(1); if(checkStop()) return;
    runPatternStep(2); if(checkStop()) return;
    runPatternStep(3); if(checkStop()) return;
    runPatternStep(4); if(checkStop()) return;
  } else {
    // 何もしていない時は安全のため確実に止める
    stopAll();
    delay(10); // 省電力
  }
}