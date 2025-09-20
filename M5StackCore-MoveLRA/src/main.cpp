#include <Arduino.h>
#include "M5Unified.h"

int D1 = 21;
int D2 = 22;
int A1 = 26;
int value[320] = {0};

void addValue(int v){
  for(int i=0;i<319;i++){
    value[i] = value[i+1];
  }
  value[319] = v;
}

void showValue(){
  M5.Lcd.fillRect(0, 0, 320, 240, BLACK);
  for(int i=0;i<319;i++){
    // M5.Lcd.drawLine(i, 240-value[i], i+1, 240-value[i+1], WHITE);
    M5.Lcd.drawPixel(i, 240-value[i], WHITE);
  }
}

void setup(){
  M5.begin();
  M5.Lcd.begin();
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(A1, OUTPUT);
  Serial.begin(9600);
}

void loop(){
  /*非対称ノコギリ波*/
  // for(uint8_t j=0;j<100;j++){
  //   M5.update();
  //   digitalWrite(D1, HIGH);
  //   digitalWrite(D2, LOW);
  //   for(int i=0;i<256;i+=2){
  //     dacWrite(A1, i);
  //     delay(0);
  //   }
  //   digitalWrite(D1, LOW);
  //   digitalWrite(D2, HIGH);
  //   for(int i=255;i>=0;i-=2){
  //     dacWrite(A1, i);
  //     delay(0);
  //   }
  // }
  // delay(500);
  // for(uint8_t j=0;j<100;j++){
  //   M5.update();
  //   digitalWrite(D1, LOW);
  //   digitalWrite(D2, HIGH);
  //   for(int i=0;i<256;i+=2){
  //     dacWrite(A1, i);
  //     // delay(0);
  //   }
  //   digitalWrite(D1, HIGH);
  //   digitalWrite(D2, LOW);
  //   for(int i=255;i>=0;i-=2){
  //     dacWrite(A1, i);
  //     // delay(0);
  //   }
  // }
  // delay(500);

  /*対象三角波*/
  digitalWrite(D2, LOW);
  digitalWrite(D1, HIGH);
  for(uint8_t j=0;j<=255;j+=1){
    dacWrite(A1, j);
    delay(0);
  }
  for(uint8_t j=255;j>=0;j-=1){
    dacWrite(A1, j);
    delay(0);
  }
  
  digitalWrite(D1, LOW);
  digitalWrite(D2, HIGH);
  for(uint8_t j=0;j<=255;j+=1){
    dacWrite(A1, j);
    delay(0);
  }
  for(uint8_t j=255;j>=0;j-=1){
    dacWrite(A1, j);
    delay(0);
  }
}