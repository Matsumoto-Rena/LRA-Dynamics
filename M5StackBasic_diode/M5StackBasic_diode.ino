#include <M5Stack.h>

const int IN1_Pin = 1;
const int IN2_Pin = 3;
const int Vref_Pin = 26;

float time = 0;
int voltage = 0;
const int max_voltage = 90.0;

void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Power.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  voltage = (int)(max_voltage * sin(time));
  
  if(voltage < 0) {
    digitalWrite(IN1_Pin, LOW);
    digitalWrite(IN2_Pin, HIGH);
    dacWrite(Vref_Pin, -1*voltage);
  } else {
    digitalWrite(IN1_Pin, HIGH);
    digitalWrite(IN2_Pin, LOW);
    dacWrite(Vref_Pin, voltage);
  }

  time += 0.001;
}
