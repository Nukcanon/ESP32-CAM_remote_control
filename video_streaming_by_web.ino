#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include <Preferences.h>
#include "soc/soc.h"           
#include "soc/rtc_cntl_reg.h"  
// 핀 정의 (AI-Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define COMM_TX_PIN 13  // 데이터 전송용
#define COMM_RX_PIN 14  // 데이터 전송용

Preferences preferences;
httpd_handle_t camera_httpd = NULL;

String ssid = "";
String password = "";
int wifiMode = 1; 

// MAC 주소 기반 SSID 생성
String getMacSSID() {
  WiFi.mode(WIFI_AP); 
  String mac = WiFi.macAddress();
  mac.replace(":", ""); 
  // 뒤에서 4글자 추출
  String suffix = mac.substring(mac.length() - 4);
  return "ESP32-CAM_" + suffix;
}

// 조이스틱 웹페이지
const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <style>
    body { font-family: Arial; text-align: center; margin:0px; padding:0px; background-color: #222; color: white; }
    h2 { margin: 10px; }
    img { width: 100%; max-width: 400px; transform: rotate(180deg); }
    .button-group { display: flex; justify-content: center; gap: 20px; }
    .btn { 
      width: 80px; height: 80px; 
      background-color: #444; border-radius: 50%; border: 2px solid #fff; 
      font-size: 24px; color: white; 
      touch-action: manipulation; -webkit-user-select: none;
    }
    .btn:active { background-color: #ff3333; }
  </style>
</head>
<body>
  <h2>ESP32 RC CAR</h2>
  <img src="" id="photo" >
  
  <div style="margin-top:20px;">
    <button class="btn" ontouchstart="startMove('fwd')" ontouchend="stopMove()" onmousedown="startMove('fwd')" onmouseup="stopMove()">▲</button>
  </div>
  
  <div class="button-group" style="margin-top: 10px; margin-bottom: 10px;">
    <button class="btn" ontouchstart="startMove('left')" ontouchend="stopMove()" onmousedown="startMove('left')" onmouseup="stopMove()">◀</button>
    <button class="btn" ontouchstart="stopMove()" onmousedown="stopMove()">■</button>
    <button class="btn" ontouchstart="startMove('right')" ontouchend="stopMove()" onmousedown="startMove('right')" onmouseup="stopMove()">▶</button>
  </div>

  <div>
    <button class="btn" ontouchstart="startMove('back')" ontouchend="stopMove()" onmousedown="startMove('back')" onmouseup="stopMove()">▼</button>
  </div>

  <script>
    document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/stream";
    
    var intervalId; // 반복 전송을 위한 타이머 ID

    // [누를 때] 0.2초마다 명령을 계속 보냄
    function startMove(action) {
      if(intervalId) clearInterval(intervalId); // 혹시 모를 중복 방지
      move(action); // 누르자마자 즉시 1회 전송
      intervalId = setInterval(function(){ 
        move(action); 
      }, 200); // 이후 200ms(0.2초) 마다 반복 전송
    }

    // [뗄 때] 반복 중단하고 정지 신호 보냄
    function stopMove() {
      if(intervalId) clearInterval(intervalId); // 반복 종료
      move('stop'); // 정지 명령 전송
    }

    function move(action) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/action?go=" + action, true);
      xhr.send();
    }
  </script>
</body>
</html>
)rawliteral";

// 서버 핸들러
static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  char part_buf[64];
  static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=123456789000000000000987654321";
  static const char* _STREAM_BOUNDARY = "\r\n--123456789000000000000987654321\r\n";
  static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) { delay(10); continue; }
    
    size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
    res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;
  }
  return res;
}

static esp_err_t cmd_handler(httpd_req_t *req) {
  char* buf;
  size_t buf_len = httpd_req_get_url_query_len(req) + 1;
  char variable[32] = {0,};
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      httpd_query_key_value(buf, "go", variable, sizeof(variable));
    }
    free(buf);
  }

  if (strlen(variable) > 0) {
    Serial.print("Action: ");
    Serial.println(variable);
    if(!strcmp(variable, "fwd")) { 
      Serial1.println("F"); // 전진
    }
    else if(!strcmp(variable, "back")) { 
      Serial1.println("B"); // 후진
    }
    else if(!strcmp(variable, "left")) { 
      Serial1.println("L"); // 좌회전
    }
    else if(!strcmp(variable, "right")) { 
      Serial1.println("R"); // 우회전
    }
    else if(!strcmp(variable, "stop")) { 
      Serial1.println("S"); // 정지
    }
  }
  
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_handler, .user_ctx = NULL };
  httpd_uri_t cmd_uri = { .uri = "/action", .method = HTTP_GET, .handler = cmd_handler, .user_ctx = NULL };

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
  }

  config.server_port = 81;
  config.ctrl_port += 1;
  httpd_handle_t stream_httpd = NULL;
  httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

// WiFi 설정
void configWiFi() {
  Serial.println("\n--------------------------------");
  Serial.println("      WiFi Configuration        ");
  Serial.println("--------------------------------");
  Serial.println("Select Mode:");
  Serial.println(" 1 : Connect to Router (Station)");
  Serial.println(" 2 : Direct Connection (AP Mode)");
  Serial.print("Input Mode (1 or 2) > ");

  while (Serial.available() == 0) delay(10);
  int mode = Serial.parseInt();
  delay(100);
  while(Serial.available()) Serial.read(); 
  
  if(mode != 1 && mode != 2) mode = 1;
  Serial.println(mode);

  if(mode == 1) {
    // 공유기 연결 모드
    Serial.println("\n[Mode 1] Router Settings");
    Serial.println("Input Router SSID: ");
    while (Serial.available() == 0) delay(10);
    ssid = Serial.readStringUntil('\n'); ssid.trim();
    Serial.println(ssid);
  } else {
    // AP 모드
    Serial.println("\n[Mode 2] AP(Hotspot) Settings");
    ssid = getMacSSID();
    Serial.print("SSID: ");
    Serial.println(ssid);
  }

  Serial.println("Input Password (min 8 chars): ");
  Serial.flush();

  while (Serial.available() == 0) delay(100);
  password = Serial.readStringUntil('\n');
  password.trim();
  Serial.println(password);

  preferences.begin("wifi-config", false);
  preferences.putInt("mode", mode);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();

  Serial.println("\nSaved! Restarting...");
  delay(1000);
  ESP.restart();
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);

  Serial1.begin(9600, SERIAL_8N1, COMM_RX_PIN, COMM_TX_PIN);

  preferences.begin("wifi-config", true);
  wifiMode = preferences.getInt("mode", 1); 
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();

  if(ssid == "") configWiFi();

  WiFi.setSleep(false);
  
  Serial.println("To re-configure, please send 'setup' via the Serial Monitor.");

  if (wifiMode == 2) {
    // AP 모드
    Serial.println("\n[Starting AP Mode]");
    WiFi.softAP(ssid.c_str(), password.c_str());
    Serial.print("AP Created! SSID: "); Serial.println(ssid);
    Serial.print("Connect your phone here and go to: http://"); Serial.println(WiFi.softAPIP());
  } else {
    // Station 모드
    Serial.println("\n[Connecting to Router]");
    WiFi.begin(ssid.c_str(), password.c_str());
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500); Serial.print(".");
      if(++retry > 30) { Serial.println("\nConnection Timeout!"); configWiFi(); }
      if(Serial.available()) {
        if(Serial.readStringUntil('\n').substring(0,5).equalsIgnoreCase("setup")) configWiFi();
      }
    }
    Serial.println("\nConnected!");
    Serial.print("Go to: http://"); Serial.println(WiFi.localIP());
  }

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  esp_camera_init(&config);
  
  sensor_t * s = esp_camera_sensor_get();
  //s->set_vflip(s, 1);
  s->set_hmirror(s, 1);

  startCameraServer();
}

void loop() {
  if(Serial.available()) {
    String s = Serial.readStringUntil('\n'); s.trim();
    if(s.equalsIgnoreCase("setup")) configWiFi();
  }
  delay(100);
}