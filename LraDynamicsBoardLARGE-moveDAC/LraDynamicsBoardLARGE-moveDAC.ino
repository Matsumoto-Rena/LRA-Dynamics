/*
LraBoard_Largeのコードです．
一方から正弦波，もう一方からノコギリ波を出力します．
*/

#include <Adafruit_MCP4725.h>

int LED = 2;

Adafruit_MCP4725 dac1;
Adafruit_MCP4725 dac2;

const PROGMEM uint16_t DACLookup_FullSine_5Bit[32] =
{
  2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
  4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
  2048, 1648, 1264,  910,  600,  345,  156,   39,
     0,   39,  156,  345,  600,  910, 1264, 1648
};


void setup() {
  // put your setup code here, to run once:
  pinMode(LED,OUTPUT);
  dac1.begin(0x62);
  dac2.begin(0x63);
  Serial.begin(9600);
}

void loop() {
  uint16_t i;
  for(i=0;i<32;i++){
    dac1.setVoltage(pgm_read_word(&(DACLookup_FullSine_5Bit[i])),false);
    dac2.setVoltage(i * (4096/32),false);
    Serial.println(i);
  }
}
