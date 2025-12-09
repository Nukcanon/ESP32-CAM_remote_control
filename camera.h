#pragma once

#include <esp_camera.h>
#include "pins.h"

class ESP32Camera {
public:
    ESP32Camera() :
        _pixelFormat(PIXFORMAT_RGB565) {
    }

    uint8_t getDepth() {
        return _pixelFormat == PIXFORMAT_GRAYSCALE ? 1 : 3;
    }

    // 기본 화질(jpegQuality)을 12로, 프레임버퍼는 1개로 제한
    bool begin(framesize_t frameSize, pixformat_t pixelFormat = PIXFORMAT_RGB565, uint8_t jpegQuality = 30) {
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
        
        // [안정화 1] 클럭 속도 20MHz (20MHz는 불안정할 수 있음)
        config.xclk_freq_hz = 20000000; 
        
        config.pixel_format = _pixelFormat = pixelFormat;
        config.frame_size = frameSize;
        config.jpeg_quality = jpegQuality;
        config.fb_count = 2; // 버퍼 개수 2개 (메모리 절약)

        // [안정화 2] OV2640은 PSRAM이 필수입니다.
        // AI-Thinker 보드는 PSRAM이 있으므로 반드시 이를 사용하도록 설정
        if(psramFound()){
            config.fb_location = CAMERA_FB_IN_PSRAM; // 버퍼를 PSRAM에 할당
            config.grab_mode = CAMERA_GRAB_LATEST;
        } else {
            // PSRAM 인식이 안 되면 일반 RAM을 쓰되 매우 불안정할 수 있음
            config.fb_location = CAMERA_FB_IN_DRAM;
        }

        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            return false;
        }

        sensor_t *sensor = esp_camera_sensor_get();
        if (sensor) {
            sensor->set_framesize(sensor, frameSize);
            // [안정화 3] 화면 반전/회전 (필요시 주석 해제)
            // sensor->set_vflip(sensor, 1);
            sensor->set_hmirror(sensor, 1);
        }

        return true;
    }

    camera_fb_t* capture() {
        return (_frame = esp_camera_fb_get());
    }

protected:
    pixformat_t _pixelFormat;
    camera_fb_t *_frame;
};