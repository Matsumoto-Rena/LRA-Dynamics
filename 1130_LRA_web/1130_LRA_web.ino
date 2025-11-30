#include <Adafruit_MCP4725.h>
#include <WiFi.h>
#include <WebServer.h>

// ★ここにWi-Fi情報を入力してください
const char* ssid = "(･∀･)";
const char* password = "235711131719";

Adafruit_MCP4725 dac1;
WebServer server(80);

// --- パラメータ変数の定義 ---
// パターン1用 (前半)
int paramRes1 = 5;   // 波の分解能 (初期値: 5)
int paramRep1 = 4;   // 内部の繰り返し回数 (初期値: 4)

// パターン2用 (後半)
int paramRes2 = 10;  // 波の分解能 (初期値: 10)
int paramRep2 = 4;   // 内部の繰り返し回数 (初期値: 4)

int loopCount = 200; // 全体のループ回数 (初期値: 200)

/*
DA1 2
DA2 3
DB1 7
DB2 10
*/

// 引数に repeatCount を追加しました
void move(int res, int repeatCount, bool isDir){
  // 安全対策
  if(res <= 0) res = 1;
  if(repeatCount < 0) repeatCount = 0;

  delayMicroseconds(100);
  
  // 指定された回数(repeatCount)だけ繰り返す
  for(int i = 0; i < repeatCount; i++){
    if(isDir){
      digitalWrite(D2,HIGH);
      digitalWrite(D3,LOW);
    }else{
      digitalWrite(D2,LOW);
      digitalWrite(D3,HIGH);
    }
    for(uint8_t k=0; k<res; k++){
      dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
    }
  }

  delayMicroseconds(100);
  if(isDir){
    digitalWrite(D2,LOW);
    digitalWrite(D3,HIGH);
  }else{
    digitalWrite(D2,HIGH);
    digitalWrite(D3,LOW);
  }
  
  // ここの最後の一回は元のコード通り1回だけ実行でよいか、
  // あるいはここも繰り返すべきか不明でしたが、元の構造を維持して1回実行にしています。
  for(uint8_t k=0; k<res; k++){
    dac1.setVoltage(4095 * 1.0 * fabs(sin(k * 2 * 3.14 / res / 2)), false, 800000);
  }
}

// Web画面の設定
void handleRoot() {
  String html = "<html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>LRA Controller</title>";
  html += "<style>body{font-family:sans-serif; padding:20px;} input{padding:5px; width:80px;}</style></head><body>";
  html += "<h2>LRA Control Panel</h2>";
  html += "<form action='/set' method='GET'>";
  
  html += "<h3>Pattern 1 (Res1 & Rep1)</h3>";
  html += "Res: <input type='number' name='res1' value='" + String(paramRes1) + "'> ";
  html += "Rep: <input type='number' name='rep1' value='" + String(paramRep1) + "'><br>";
  
  html += "<h3>Pattern 2 (Res2 & Rep2)</h3>";
  html += "Res: <input type='number' name='res2' value='" + String(paramRes2) + "'> ";
  html += "Rep: <input type='number' name='rep2' value='" + String(paramRep2) + "'><br>";
  
  html += "<h3>Main Loop</h3>";
  html += "Loop Count: <input type='number' name='loops' value='" + String(loopCount) + "'><br><br>";
  
  html += "<input type='submit' value='UPDATE' style='width:100%; height:50px; background:#ddd;'></form>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// パラメータ更新処理
void handleSet() {
  if (server.hasArg("res1")) paramRes1 = server.arg("res1").toInt();
  if (server.hasArg("rep1")) paramRep1 = server.arg("rep1").toInt(); // Rep1更新
  
  if (server.hasArg("res2")) paramRes2 = server.arg("res2").toInt();
  if (server.hasArg("rep2")) paramRep2 = server.arg("rep2").toInt(); // Rep2更新
  
  if (server.hasArg("loops")) loopCount = server.arg("loops").toInt();
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);

  pinMode(D2,OUTPUT);
  pinMode(D3,OUTPUT);
  pinMode(D6,OUTPUT);
  pinMode(D7,OUTPUT);
  pinMode(D8,OUTPUT);
  pinMode(D10,OUTPUT);

  dac1.begin(0x62);
  
  digitalWrite(D2,HIGH);
  digitalWrite(D3,LOW);
  digitalWrite(D6,LOW);
  digitalWrite(D7,LOW);
  digitalWrite(D8,LOW);
  digitalWrite(D10,LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nIP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}

void loop() {
  server.handleClient();

  // --- パターン1: Res1 と Rep1 を使用 ---
  digitalWrite(D6, HIGH);
  digitalWrite(D8, LOW);
  digitalWrite(D7, LOW);
  digitalWrite(D10, LOW);
  for(int j = 0; j < loopCount; j++){
    move(paramRes1, paramRep1, true); // 変数を渡す
    if(j % 10 == 0) server.handleClient();
  }

  // --- パターン1 (逆): Res1 と Rep1 を使用 ---
  digitalWrite(D6, LOW);
  digitalWrite(D8, HIGH);
  digitalWrite(D7, LOW);
  digitalWrite(D10, LOW);
  for(int j = 0; j < loopCount; j++){
    move(paramRes1, paramRep1, false); // 変数を渡す
    if(j % 10 == 0) server.handleClient();
  }

  // --- パターン2: Res2 と Rep2 を使用 ---
  digitalWrite(D6, LOW);
  digitalWrite(D8, LOW);
  digitalWrite(D7, HIGH);
  digitalWrite(D10, LOW);
  for(int j = 0; j < loopCount; j++){
    move(paramRes2, paramRep2, true); // 変数を渡す
    if(j % 10 == 0) server.handleClient();
  }

  // --- パターン2 (逆): Res2 と Rep2 を使用 ---
  digitalWrite(D6, LOW);
  digitalWrite(D8, LOW);
  digitalWrite(D7, LOW);
  digitalWrite(D10,HIGH);
  for(int j = 0; j < loopCount; j++){
    move(paramRes2, paramRep2, false); // 変数を渡す
    if(j % 10 == 0) server.handleClient();
  }
}