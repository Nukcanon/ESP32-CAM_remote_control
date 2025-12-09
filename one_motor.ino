// 1. 오른쪽 바퀴 핀 (16번, 17번)
const int motorRight_IN1 = 16;
const int motorRight_IN2 = 17;

void setup() {
  // 2. 핀을 출력 모드로 설정
  pinMode(motorRight_IN1, OUTPUT);
  pinMode(motorRight_IN2, OUTPUT);
}

void loop() {
  // [동작 1] 앞으로 돌리기 (정회전)
  digitalWrite(motorRight_IN1, HIGH);
  digitalWrite(motorRight_IN2, LOW);
  delay(2000); // 2초 유지

  // [동작 2] 멈추기
  digitalWrite(motorRight_IN1, LOW);
  digitalWrite(motorRight_IN2, LOW);
  delay(1000); // 1초 정지

  // [동작 3] 뒤로 돌리기 (역회전)
  digitalWrite(motorRight_IN1, LOW);
  digitalWrite(motorRight_IN2, HIGH);
  delay(2000); // 2초 유지

  // 다시 멈춤
  digitalWrite(motorRight_IN1, LOW);
  digitalWrite(motorRight_IN2, LOW);
  delay(1000);
}