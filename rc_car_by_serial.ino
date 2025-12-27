#include <Preferences.h>

// 통신용 RX 핀 정의
const int RX_PIN = 13; // 받기 (CAM의 13번과 연결)
const int TX_PIN = 14; // 보내기 (CAM의 14번과 연결)

unsigned long lastCmdTime = 0;
void initMotor();
void goForward(long spd);
void goBackward(long spd);
void turnRight(long spd);
void turnLeft(long spd);
void stopMotor();

Preferences prefs;

// 속도 및 밸런스 설정 변수
long speedFwd = 1023;   
long speedBwd = 1023;   
long speedLeft = 1023;
long speedRight = 1023; 
int motorBalance = 0;   

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  prefs.begin("car_config", false);

  speedFwd = prefs.getLong("fwd", 1023);
  speedBwd = prefs.getLong("bwd", 1023);
  speedLeft = prefs.getLong("left", 1023);
  speedRight = prefs.getLong("right", 1023);
  motorBalance = prefs.getInt("bal", 0);

  initMotor();
  
  Serial.println("\n\n");
  Serial.println("===================================================");
  Serial.println("       ESP32 RC Car Control & Configuration        ");
  Serial.println("===================================================");
  Serial.println(" [기본 정보]");
  Serial.println(" * 속도 범위 : 0 ~ 1023 (1023이 최대 속도)");
  Serial.println(" * 밸런스 범위: -50 ~ 50 (%) (0이 중앙)");
  Serial.println("   - 양수(+) 값 (예: bal 10) : 오른쪽 바퀴가 더 빠름 (왼쪽 감속)");
  Serial.println("   - 음수(-) 값 (예: bal -10): 왼쪽 바퀴가 더 빠름 (오른쪽 감속)");
  Serial.println("---------------------------------------------------");
  Serial.println(" [명령어 사용법 (띄어쓰기 상관 없음)]");
  Serial.println(" f300 또는 f 300 : 전진 속도 설정");
  Serial.println(" b300 또는 b 300 : 후진 속도 설정");
  Serial.println(" l300 또는 l 300 : 좌회전 속도 설정");
  Serial.println(" r300 또는 r 300 : 우회전 속도 설정");
  Serial.println(" bal10 또는 bal 10 : 밸런스 조절 (-50 ~ 50 %)");
  Serial.println(" show            : 현재 설정값 확인");
  Serial.println(" reset           : 설정 초기화 (최대 속도, 밸런스 0)");
  Serial.println("===================================================");
  Serial.printf(" [불러온 설정]\n F:%ld, B:%ld, L:%ld, R:%ld, Bal:%d%%\n", speedFwd, speedBwd, speedLeft, speedRight, motorBalance);
  Serial.println("Motor Ready (Waiting for commands...)");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    String lowerInput = input;
    lowerInput.toLowerCase();

    if (lowerInput.length() == 0) return; 
    if (lowerInput == "show") {
      String balDir = "Center";
      if (motorBalance > 0) balDir = "Right Faster";
      if (motorBalance < 0) balDir = "Left Faster";

      Serial.printf("[Current Settings]\n Fwd:%ld, Bwd:%ld, Left:%ld, Right:%ld\n Balance:%d%% (%s)\n", 
                    speedFwd, speedBwd, speedLeft, speedRight, motorBalance, balDir.c_str());
      Serial.println("All settings are saved in Flash memory.");
    }
    else if (lowerInput == "reset") {
       speedFwd = 1023; speedBwd = 1023; speedLeft = 1023; speedRight = 1023; motorBalance = 0;
       prefs.putLong("fwd", speedFwd);
       prefs.putLong("bwd", speedBwd);
       prefs.putLong("left", speedLeft);
       prefs.putLong("right", speedRight);
       prefs.putInt("bal", motorBalance);
       Serial.println("Settings reset to default.");
    }
    else if (lowerInput.startsWith("bal")) {
      String valStr = lowerInput.substring(3); 
      valStr.trim(); 
      
      int val = valStr.toInt();
      if (val > 50) val = 50;
      if (val < -50) val = -50;
      
      motorBalance = val;
      prefs.putInt("bal", motorBalance); 
      
      String dir = (val > 0) ? "Right Faster (Left slows down)" : (val < 0) ? "Left Faster (Right slows down)" : "Center";
      Serial.println("Set Balance: " + String(motorBalance) + "% -> " + dir + " (Saved)");
    }
    else {
      char cmd = lowerInput.charAt(0); 
      String valStr = lowerInput.substring(1); 
      valStr.trim(); 
      
      long val = valStr.toInt();
      if(val > 1023) val = 1023;
      if(val < 0) val = 0;       

      if (cmd == 'f') {
        speedFwd = val;
        prefs.putLong("fwd", speedFwd); 
        Serial.println("Set Forward Speed: " + String(speedFwd) + " (Saved)");
      }
      else if (cmd == 'b') {
        speedBwd = val;
        prefs.putLong("bwd", speedBwd); 
        Serial.println("Set Backward Speed: " + String(speedBwd) + " (Saved)");
      }
      else if (cmd == 'l') {
        speedLeft = val;
        prefs.putLong("left", speedLeft); 
        Serial.println("Set Left Turn Speed: " + String(speedLeft) + " (Saved)");
      }
      else if (cmd == 'r') {
        speedRight = val;
        prefs.putLong("right", speedRight); 
        Serial.println("Set Right Turn Speed: " + String(speedRight) + " (Saved)");
      }
      else {
        Serial.println("Unknown Command.");
      }
    }
  }

  // 2. ESP32-CAM 주행 명령 처리
  if (Serial1.available()) {
    String input = Serial1.readStringUntil('\n'); 
    input.trim(); // 공백 제거

    if (input.length() > 0) {
      lastCmdTime = millis();
      
      char cmd = input.charAt(0); 
      int targetSpeed = -1;

      int commaIndex = input.indexOf(',');
      if (commaIndex != -1) {
        String speedStr = input.substring(commaIndex + 1);
        targetSpeed = speedStr.toInt();
      }

      Serial.print("Net Cmd: "); Serial.print(cmd);
      if(targetSpeed != -1) {
        Serial.print(" | Speed: "); Serial.println(targetSpeed);
      } else {
        Serial.println(" | Default Speed");
      }

      switch (cmd) {
        case 'F': 
          goForward(targetSpeed != -1 ? targetSpeed : speedFwd); 
          break;
        case 'B': 
          goBackward(targetSpeed != -1 ? targetSpeed : speedBwd); 
          break;
        case 'L': 
          turnLeft(targetSpeed != -1 ? targetSpeed : speedLeft); 
          break;
        case 'R': 
          turnRight(targetSpeed != -1 ? targetSpeed : speedRight); 
          break;
        case 'S': 
          stopMotor(); 
          break;
      }
    }
  }
  
  if (millis() - lastCmdTime > 500) {
    stopMotor();
  }
  
  delay(10); 
}