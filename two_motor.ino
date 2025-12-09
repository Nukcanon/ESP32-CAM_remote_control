// 오른쪽 바퀴 핀
const int R_IN1 = 16;
const int R_IN2 = 17;
// 왼쪽 바퀴 핀
const int L_IN3 = 21;
const int L_IN4 = 22;

void setup() {
  pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  pinMode(L_IN3, OUTPUT); pinMode(L_IN4, OUTPUT);
}

void loop() {
  // 1. 전진 (양쪽 정회전)
  digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW);
  digitalWrite(L_IN3, HIGH); digitalWrite(L_IN4, LOW);
  delay(2000); 

  // 2. 정지
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
  digitalWrite(L_IN3, LOW); digitalWrite(L_IN4, LOW);
  delay(1000);

  // 3. 제자리 우회전 (오른쪽 역회전, 왼쪽 정회전)
  digitalWrite(R_IN1, LOW);  digitalWrite(R_IN2, HIGH);
  digitalWrite(L_IN3, HIGH); digitalWrite(L_IN4, LOW);
  delay(1000); 

  // 4. 정지
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
  digitalWrite(L_IN3, LOW); digitalWrite(L_IN4, LOW);
  delay(1000);
}