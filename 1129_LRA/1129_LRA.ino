/*
LraBoard_Largeのコードです．
MCP4725ライブラリのI2Cのデフォルトは400kHzですが，800kHzで駆動させています．
また，正弦波の分解能を16段階にすることで350Hzほどで正弦波が出力できます．
*/

#include <Adafruit_MCP4725.h>


Adafruit_MCP4725 dac1;

/*
DA1 2
DA2 3
DB1 7
DB2 10
*/

const PROGMEM uint16_t DACLookup_FullSine_5Bit[16] =
{
  2048, 2831, 3495, 3939,
  4095, 3939, 3495, 2831,
  2048, 1264,  600,  156,
     0,  156,  600, 1264
};

void move(int res, bool isDir){
  // digitalWrite(D2,LOW);
  // digitalWrite(D3,LOW);
  delayMicroseconds(100);
  for(uint8_t i = 0; i < 3; i++){
    if(isDir){
      digitalWrite(D2,HIGH);
      digitalWrite(D3,LOW);
    }else{
      digitalWrite(D2,LOW);
      digitalWrite(D3,HIGH);
    }
    for(uint8_t i=0;i<res;i++){
      dac1.setVoltage(4095 * 1.0 * fabs(sin(i * 2 * 3.14 / res / 2)),false,800000);
    }
  }
  // digitalWrite(D2,LOW);
  // digitalWrite(D3,LOW);
  delayMicroseconds(100);
  if(isDir){
    digitalWrite(D2,LOW);
    digitalWrite(D3,HIGH);
  }else{
    digitalWrite(D2,HIGH);
    digitalWrite(D3,LOW);
  }
  for(uint8_t i=0;i<res;i++){
    dac1.setVoltage(4095 * 1.0 * fabs(sin(i * 2 * 3.14 / res / 2)),false,800000);
  }
}

void setup() {
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
}

void loop() {
  digitalWrite(D6, HIGH);
  digitalWrite(D8, LOW);
  digitalWrite(D7, LOW);
  digitalWrite(D10, LOW);
  for(int j = 0; j < 200 ; j++){
    move(3,true);
  }
  digitalWrite(D6, LOW);
  digitalWrite(D8, HIGH);
  digitalWrite(D7, LOW);
  digitalWrite(D10, LOW);
  for(int j = 0; j < 200 ; j++){
    move(3,false);
  }
  digitalWrite(D6, LOW);
  digitalWrite(D8, LOW);
  digitalWrite(D7, HIGH);
  digitalWrite(D10, LOW);
  for(int j = 0; j < 200 ; j++){
    move(13,true);
  }
  digitalWrite(D6, LOW);
  digitalWrite(D8, LOW);
  digitalWrite(D7, LOW);
  digitalWrite(D10,HIGH);
  for(int j = 0; j < 200 ; j++){
    move(13,false);
  }
}