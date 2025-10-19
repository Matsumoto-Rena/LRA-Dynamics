/*https://robohanactive.slack.com/archives/C08RF0U7A49/p1758517780500099
このリンク先のスレ用のコードです.
MDのアナログINにはただの山を繰り返し入力しており,
デジタルIN1,2の切り替えタイミングを調整することで
二種類の非対称波形をMDから出力することができます.
*/

#include <Arduino.h>
#include "M5Unified.h"
#include "math.h"

int D1 = 21;
int D2 = 22;
int A1 = 26;
int value[320] = {0};

int length = 81;

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
  M5.Lcd.println("Let's start!");
}

void loop(){
  M5.update();
  if(M5.BtnA.isPressed()){
    for(uint8_t j=0;j<3;j++){
      digitalWrite(D2, LOW);
      digitalWrite(D1, HIGH);
      for(int i=0;i<length;i++){
        dacWrite(A1, 255 * sin(i*3.14/length));
      }
    }
    digitalWrite(D1, LOW);
    digitalWrite(D2, HIGH);
    for(int i=0;i<length;i++){
      dacWrite(A1, 255 * sin(i*3.14/length));
    }
  }else if(M5.BtnB.isPressed()){
    for(uint8_t j=0;j<3;j++){
      digitalWrite(D1, LOW);
      digitalWrite(D2, HIGH);
      for(int i=0;i<length;i++){
        dacWrite(A1, 255 * sin(i*3.14/length));
      }
    }
    digitalWrite(D2, LOW);
    digitalWrite(D1, HIGH);
    for(int i=0;i<length;i++){
      dacWrite(A1, 255 * sin(i*3.14/length));
    }
  }
}