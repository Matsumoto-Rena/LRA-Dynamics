#include <avr/io.h>

#define PWMPin 10
int D1 = 2;
int D2 = 3;

unsigned int frq = 170; // 周波数
float duty = 0.5; // 指定したいデューティ比

void setup() {
  pinMode(PWMPin, OUTPUT);
  pinMode(D1, HIGH);
  pinMode(D2, HIGH);
  // モード指定
  TCCR1A = 0b00100001;
  TCCR1B = 0b00010010;

  // TOP値指定
  OCR1A = (unsigned int)(1000000 / frq);

  // Duty比指定
  OCR1B = (unsigned int)(1000000 / frq * duty);

}

void loop() {
  digitalWrite(D1, HIGH);
  digitalWrite(D2, LOW);
  delay(1000);
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  delay(1000);
  digitalWrite(D2, HIGH);
  digitalWrite(D1, LOW);
  delay(1000);
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  delay(1000);
}