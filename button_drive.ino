// 1. 모터 핀 설정
const int R_IN1 = 16; const int R_IN2 = 17;
const int L_IN3 = 21; const int L_IN4 = 22;

// 2. 버튼 핀 설정
const int btnFwd  = 32; // 전진 버튼
const int btnBack = 33; // 후진 버튼
const int btnLeft = 25; // 좌회전
const int btnRight= 26; // 우회전

void setup() {
  // 모터 출력 설정
  pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  pinMode(L_IN3, OUTPUT); pinMode(L_IN4, OUTPUT);

  // 버튼 입력 설정 (내부 풀업 저항 사용)
  pinMode(btnFwd, INPUT_PULLUP);
  pinMode(btnBack, INPUT_PULLUP);
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnRight, INPUT_PULLUP);
}

void loop() {
  // 버튼 상태 읽기 (눌리면 LOW)
  int fwd   = digitalRead(btnFwd);
  int back  = digitalRead(btnBack);
  int left  = digitalRead(btnLeft);
  int right = digitalRead(btnRight);

  // 입력에 따른 동작 제어
  if (fwd == LOW) {       
    goForward();
  } 
  else if (back == LOW) { 
    goBackward();
  }
  else if (left == LOW) { 
    turnLeft();
  }
  else if (right == LOW) { 
    turnRight();
  }
  else {                  
    stopCar();
  }
}

// --- 동작 함수 정의 ---

void goForward() {
  digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW);
  digitalWrite(L_IN3, HIGH); digitalWrite(L_IN4, LOW);
}

void goBackward() {
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, HIGH);
  digitalWrite(L_IN3, LOW); digitalWrite(L_IN4, HIGH);
}

void turnLeft() {
  digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW); 
  digitalWrite(L_IN3, LOW);  digitalWrite(L_IN4, HIGH); 
}

void turnRight() {
  digitalWrite(R_IN1, LOW);  digitalWrite(R_IN2, HIGH); 
  digitalWrite(L_IN3, HIGH); digitalWrite(L_IN4, LOW); 
}

void stopCar() {
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
  digitalWrite(L_IN3, LOW); digitalWrite(L_IN4, LOW);
}