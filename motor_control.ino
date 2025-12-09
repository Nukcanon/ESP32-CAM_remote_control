// 16,17: 오른쪽 바퀴 / 22,21: 왼쪽 바퀴
const int dcMotors[] = {17, 16, 21, 22};
const int mot_channels[] = {0, 1, 2, 3};
const int mot_freq = 10000; // 10kHz
const int mot_res = 10;     // 10비트 해상도 (0~1023)

const int SPEED_MIN = 256;
const int SPEED_MAX = 1023;

// rc_car_by_serial.ino에 선언된 전역 변수를 가져옴
extern int motorBalance;

void initMotor() {  
  for(int i=0; i < 4; i++) {
    // 1. 채널 설정
    ledcSetup(mot_channels[i], mot_freq, mot_res);
    // 2. 핀과 채널 연결
    ledcAttachPin(dcMotors[i], mot_channels[i]);
  }
}

// 밸런스 적용을 위한 내부 헬퍼 함수
long applyBalance(long spd, bool isRight) {
  // 밸런스가 0이면 그대로 반환
  if (motorBalance == 0) return spd;

  // motorBalance > 0 : 오른쪽 우세 (왼쪽 모터 감속)
  if (motorBalance > 0 && !isRight) {
    return spd * (100 - motorBalance) / 100;
  }
  // motorBalance < 0 : 왼쪽 우세 (오른쪽 모터 감속)
  else if (motorBalance < 0 && isRight) {
    return spd * (100 - abs(motorBalance)) / 100;
  }
  
  return spd;
}

void goForward(long spd) {
  if(spd < 0) spd = 0;
  
  // 좌우 속도 각각 계산 (밸런스 적용)
  long spdR = applyBalance(spd, true);
  long spdL = applyBalance(spd, false);

  // 최소 속도 보정 및 최대값 제한
  spdR += SPEED_MIN; if(spdR > SPEED_MAX) spdR = SPEED_MAX;
  spdL += SPEED_MIN; if(spdL > SPEED_MAX) spdL = SPEED_MAX;

  // 오른쪽 바퀴 (채널 0, 1) [cite: 7]
  ledcWrite(mot_channels[0], SPEED_MAX);       
  ledcWrite(mot_channels[1], SPEED_MAX - spdR);
  
  // 왼쪽 바퀴 (채널 2, 3)
  ledcWrite(mot_channels[2], SPEED_MAX);       
  ledcWrite(mot_channels[3], SPEED_MAX - spdL);
}

void goBackward(long spd) {
  if(spd < 0) spd = 0;

  // 좌우 속도 각각 계산 (밸런스 적용)
  long spdR = applyBalance(spd, true);
  long spdL = applyBalance(spd, false);

  // 최소 속도 보정 및 최대값 제한
  spdR += SPEED_MIN; if(spdR > SPEED_MAX) spdR = SPEED_MAX;
  spdL += SPEED_MIN; if(spdL > SPEED_MAX) spdL = SPEED_MAX;

  // 오른쪽 바퀴 후진 [cite: 10]
  ledcWrite(mot_channels[0], SPEED_MAX - spdR);
  ledcWrite(mot_channels[1], SPEED_MAX);
  
  // 왼쪽 바퀴 후진
  ledcWrite(mot_channels[2], SPEED_MAX - spdL);
  ledcWrite(mot_channels[3], SPEED_MAX);
}

void stopMotor() { 
  for(int i=0; i < 4; i++) {
    ledcWrite(mot_channels[i], 0);
  }
}

// 회전은 밸런스 적용 없이 설정된 속도로 강하게 회전
void turnLeft(long spd) {
  if(spd < 0) spd = 0;
  spd += SPEED_MIN;
  if(spd > SPEED_MAX) spd = SPEED_MAX;

  // 오른쪽 전진
  ledcWrite(mot_channels[0], SPEED_MAX);
  ledcWrite(mot_channels[1], SPEED_MAX - spd);

  // 왼쪽 후진 (제자리 회전)
  ledcWrite(mot_channels[2], SPEED_MAX - spd); 
  ledcWrite(mot_channels[3], SPEED_MAX);
}

void turnRight(long spd) {
  if(spd < 0) spd = 0;
  spd += SPEED_MIN;
  if(spd > SPEED_MAX) spd = SPEED_MAX;

  // 오른쪽 후진
  ledcWrite(mot_channels[0], SPEED_MAX - spd);
  ledcWrite(mot_channels[1], SPEED_MAX);

  // 왼쪽 전진
  ledcWrite(mot_channels[2], SPEED_MAX);
  ledcWrite(mot_channels[3], SPEED_MAX - spd);
}