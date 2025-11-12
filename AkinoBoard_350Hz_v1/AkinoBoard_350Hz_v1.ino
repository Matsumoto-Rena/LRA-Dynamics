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

void setup() {
  pinMode(D2,OUTPUT);
  pinMode(D3,OUTPUT);
  pinMode(D7,OUTPUT);
  pinMode(D10,OUTPUT);
  dac1.begin(0x62);
  digitalWrite(D2,HIGH);
  digitalWrite(D3,LOW);
  digitalWrite(D7,HIGH);
  digitalWrite(D10,LOW);
}

void loop() {
  uint16_t i;
  for(i=0;i<16;i++){
    dac1.setVoltage(pgm_read_word(&(DACLookup_FullSine_5Bit[i])),false,800000);
  }
}