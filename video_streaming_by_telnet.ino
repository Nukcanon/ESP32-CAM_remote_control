#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "WiFi.h"
#include <Preferences.h>

// AI-Thinker 모델 사용
#define CAMERA_MODEL_AI_THINKER
#include "camera.h"

WiFiServer camServer(80);
WiFiServer motServer(81);
ESP32Camera camera;
Preferences preferences;

// 데이터 전송용 핀 (ESP32 <-> ESP32-CAM 통신)
#define COMM_TX_PIN 13
#define COMM_RX_PIN 14

String ssid = "";
String password = "";
int wifiMode = 1; // 1: Station(공유기), 2: AP(핫스팟)

void motorTask(void * parameter);

// ---------------------------------------------------------------
// MAC 주소 기반 SSID 생성 (AP 모드용)
// ---------------------------------------------------------------
String getMacSSID() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  char macStr[20];
  sprintf(macStr, "ESP32-CAM-%02X%02X", mac[4], mac[5]); // 뒤 4자리 활용
  return String(macStr);
}

// ---------------------------------------------------------------
// WiFi 설정 메뉴 (시리얼 모니터)
// ---------------------------------------------------------------
void configWiFi() {
  Serial.println("\n\n========================================");
  Serial.println("      WiFi Configuration Mode           ");
  Serial.println("========================================");
  Serial.println("Select Mode:");
  Serial.println(" 1 : Connect to Router (Station Mode)");
  Serial.println(" 2 : Create Hotspot (AP Mode)");
  Serial.print("Input Mode (1 or 2) > ");

  while (Serial.available() == 0) delay(10);
  int mode = Serial.parseInt();
  while(Serial.available()) Serial.read(); // 버퍼 비우기
  
  if(mode != 1 && mode != 2) mode = 1;
  Serial.println(mode);

  if(mode == 1) {
    // [모드 1] 공유기 연결
    Serial.println("\n[Mode 1] Connect to Router");
    Serial.println("Input Router SSID > ");
    while (Serial.available() == 0) delay(10);
    ssid = Serial.readStringUntil('\n'); ssid.trim();
    Serial.println(ssid);
  } else {
    // [모드 2] AP 모드 (자동 생성)
    Serial.println("\n[Mode 2] Create Access Point");
    ssid = getMacSSID();
    Serial.print("SSID: ");
    Serial.println(ssid);
  }

  Serial.println("Input Password (min 8 chars) > ");
  Serial.flush();

  while (Serial.available() == 0) delay(100);
  password = Serial.readStringUntil('\n'); password.trim();
  Serial.println(password);

  // 설정값 저장
  preferences.begin("wifi-config", false);
  preferences.putInt("mode", mode);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();

  Serial.println("\n[Success] Saved! Restarting system...");
  delay(1000);
  ESP.restart();
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  
  // 자동차(ESP32)와 통신하기 위한 Serial1 시작
  Serial1.begin(9600, SERIAL_8N1, COMM_RX_PIN, COMM_TX_PIN);
  
  delay(2000);

  // 1. 카메라 초기화
  Serial.print("Initializing Camera... ");
  // QQVGA, QVGA, VGA 등 해상도 선택
  if (camera.begin(FRAMESIZE_QVGA, PIXFORMAT_JPEG)) {
    Serial.println("OK!");
  } else {
    Serial.println("FAILED!");
    Serial.println("Check camera module connection.");
  }

  // 2. 저장된 WiFi 설정 불러오기
  preferences.begin("wifi-config", true);
  wifiMode = preferences.getInt("mode", 0);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();

  // 설정이 없거나 SSID가 비어있으면 설정 모드 진입
  if (wifiMode == 0 || ssid == "") {
    configWiFi();
  }

  // 3. WiFi 연결 시작
  if (wifiMode == 2) {
    // --- AP 모드 ---
    Serial.println("\n[Starting AP Mode]");
    WiFi.softAP(ssid.c_str(), password.c_str());
    
    Serial.print("AP Created! Connect to SSID: "); Serial.println(ssid);
    Serial.print("Server IP: "); Serial.println(WiFi.softAPIP());
  } else {
    // --- Station 모드 ---
    Serial.println("\n[Connecting to Router]");
    Serial.print("SSID: "); Serial.println(ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500); Serial.print(".");
      if (++retry > 30) { // 15초 이상 연결 안 되면 설정 모드로
        Serial.println("\nConnection Timeout! Entering Setup...");
        configWiFi();
      }
      // 연결 시도 중에도 'setup' 입력 감지
      if (Serial.available()) {
        String input = Serial.readStringUntil('\n'); input.trim();
        if (input.equalsIgnoreCase("setup")) configWiFi();
      }
    }
    Serial.println("\nConnected!");
    Serial.print("Server IP: "); Serial.println(WiFi.localIP());
  }

  // 4. 서버 시작
  camServer.begin();
  motServer.begin();

  Serial.println("\n--------------------------------------------------");
  Serial.println("System Ready. To re-configure, type 'setup'.");
  Serial.printf("Video Stream : %s:80\n", (wifiMode==2 ? WiFi.softAPIP() : WiFi.localIP()).toString().c_str());
  Serial.printf("Motor Control: %s:81\n", (wifiMode==2 ? WiFi.softAPIP() : WiFi.localIP()).toString().c_str());
  Serial.println("--------------------------------------------------");

  // 모터 제어 태스크 시작
  xTaskCreate(motorTask, "motor Task", 10000, NULL, 1, NULL);
}

void loop() {
  // 시리얼 모니터에서 'setup' 입력 시 설정 모드 진입
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.equalsIgnoreCase("setup")) {
      camServer.stop();
      motServer.stop();
      configWiFi();
    }
  }

  // 영상 스트리밍 처리 (Port 80)
  WiFiClient client = camServer.available();
  if (client) {
    while (client.connected()) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) { delay(10); continue; }

      if(client.available()) {
        char cmd = client.read();
        if(cmd == 12) {
          client.write((const uint8_t *)&fb->len, sizeof(fb->len));
          client.write((const uint8_t *)fb->buf, fb->len);
        }
      }
      esp_camera_fb_return(fb);
    }
  }
}

// 모터 제어 명령 처리 태스크
void motorTask(void * parameter) {
  for(;;) {
    WiFiClient client = motServer.available();
    if (client) {
      client.setNoDelay(true);
      while (client.connected()) {
        if(client.available()) {
          // '\n' (줄바꿈)이 나올 때까지 데이터를 읽어서 한 번에 전송
          String cmdStr = client.readStringUntil('\n');
          cmdStr.trim(); // 공백 제거

          if (cmdStr.length() > 0) {
            Serial1.println(cmdStr);
            // Serial.println("To Car: " + cmdStr); 
          }
        }
      }
    }
    delay(10);
  }
}