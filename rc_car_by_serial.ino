// ESP32 Dev Module + Core 2.0.17 기준

// 통신용 RX 핀 정의
const int RX_PIN = 13; // 받기 (CAM의 13번과 연결)
const int TX_PIN = 14; // 보내기 (CAM의 14번과 연결)

unsigned long lastCmdTime = 0;

// motor_control.ino에 있는 함수들 선언
void initMotor();
void goForward(long spd);
void goBackward(long spd);
void turnRight(long spd);
void turnLeft(long spd);
void stopMotor();

// 속도 및 밸런스 설정 변수
long speedFwd = 1023;   
long speedBwd = 1023;   
long speedLeft = 1023;  
long speedRight = 1023; 
int motorBalance = 0;   

void setup() {
  Serial.begin(115200);

  // 시리얼 통신 초기화
  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // 모터 초기화
  initMotor();
  
  Serial.println("\n\n");
  Serial.println("===================================================");
  Serial.println("       ESP32 RC Car Control & Configuration        ");
  Serial.println("===================================================");
  Serial.println(" [기본 정보]");
  Serial.println(" * 속도 범위 : 0 ~ 1023 (1023이 최대 속도/풀파워)");
  Serial.println(" * 밸런스 범위: -50 ~ 50 (0이 중앙)");
  Serial.println("---------------------------------------------------");
  Serial.println(" [명령어 사용법]");
  Serial.println(" f <값>   : 전진 속도 설정 (예: f 1023)");
  Serial.println(" b <값>   : 후진 속도 설정 (예: b 800)");
  Serial.println(" l <값>   : 좌회전 속도 설정");
  Serial.println(" r <값>   : 우회전 속도 설정");
  Serial.println(" bal <값> : 좌우 밸런스 조절(퍼센트 값) (예: bal 10)");
  Serial.println("            (+값: 오른쪽 우세 / -값: 왼쪽 우세)");
  Serial.println(" show     : 현재 설정값 확인");
  Serial.println("===================================================");
  Serial.println("Motor Ready (Waiting for commands...)");
}

void loop() {
  // 1. 시리얼 모니터(PC) 설정 명령어 처리
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim(); 

    if (input.equalsIgnoreCase("show")) {
      Serial.printf("[Current Settings]\n Fwd:%ld, Bwd:%ld, Left:%ld, Right:%ld\n Balance:%d (Max:1023)\n", 
                    speedFwd, speedBwd, speedLeft, speedRight, motorBalance);
    } 
    else if (input.startsWith("f ")) {
      speedFwd = input.substring(2).toInt();
      if(speedFwd > 1023) speedFwd = 1023; // 최대값 안전장치
      Serial.println("Set Forward Speed: " + String(speedFwd));
    }
    else if (input.startsWith("b ")) {
      speedBwd = input.substring(2).toInt();
      if(speedBwd > 1023) speedBwd = 1023;
      Serial.println("Set Backward Speed: " + String(speedBwd));
    }
    else if (input.startsWith("l ")) {
      speedLeft = input.substring(2).toInt();
      if(speedLeft > 1023) speedLeft = 1023;
      Serial.println("Set Left Turn Speed: " + String(speedLeft));
    }
    else if (input.startsWith("r ")) {
      speedRight = input.substring(2).toInt();
      if(speedRight > 1023) speedRight = 1023;
      Serial.println("Set Right Turn Speed: " + String(speedRight));
    }
    else if (input.startsWith("bal ")) {
      motorBalance = input.substring(4).toInt();
      if (motorBalance > 50) motorBalance = 50;
      if (motorBalance < -50) motorBalance = -50;
      Serial.println("Set Balance: " + String(motorBalance));
    }
    else {
      if(input.length() > 0) Serial.println("Unknown Command.");
    }
  }

  // 2. ESP32-CAM 주행 명령 처리
  if (Serial1.available()) {
    char cmd = Serial1.read(); 
    lastCmdTime = millis();

    Serial.print("Command: "); Serial.println(cmd);

    switch (cmd) {
      case 'F': goForward(speedFwd); break;
      case 'B': goBackward(speedBwd); break;
      case 'L': turnLeft(speedLeft); break;
      case 'R': turnRight(speedRight); break;
      case 'S': stopMotor(); break;
    }
  }
  
  // 3. 안전장치 (0.5초 타임아웃)
  if (millis() - lastCmdTime > 500) {
    stopMotor();
  }
  
  delay(10); 
}