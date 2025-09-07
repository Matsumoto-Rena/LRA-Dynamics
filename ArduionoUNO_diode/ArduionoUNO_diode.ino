const int IN1_Pin = 12;
const int IN2_Pin = 13;
const int Vref_Pin = 3;

float time = 0;
int voltage = 0;
const int max_voltage = 90.0;

void setup() {
  // put your setup code here, to run once:
  pinMode(IN1_Pin, OUTPUT);
  pinMode(IN2_Pin, OUTPUT);
  pinMode(Vref_Pin, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  voltage = (int)(max_voltage * sin(time));
  
  if(voltage < 0) {
    digitalWrite(IN1_Pin, LOW);
    digitalWrite(IN2_Pin, HIGH);
    analogWrite(Vref_Pin, -1*voltage);
  } else {
    digitalWrite(IN1_Pin, HIGH);
    digitalWrite(IN2_Pin, LOW);
    analogWrite(Vref_Pin, voltage);
  }

  time += 0.001;
}
