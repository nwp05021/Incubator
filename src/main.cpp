#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// LovyanGFX 라이브러리 포함
#include <LovyanGFX.hpp>

// 로깅을 위한 태그 정의
static const char *TAG = "MAIN";

/* 
 * [문서화] LGFX 설정 클래스
 * 사용하시는 디스플레이의 드라이버 IC(예: ST7789, ILI9341)와 
 * 핀 맵핑을 이 클래스 내부에서 설정해야 합니다.
 * 아래는 일반적인 SPI 디스플레이(예: ST7789)를 위한 뼈대(Template)입니다.
 */
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI      _bus_instance;

public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();
            // 사용하는 보드에 맞게 SPI 핀 번호를 반드시 수정하세요!
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.pin_sclk = 12; // SCK 핀
            cfg.pin_mosi = 11; // MOSI 핀
            cfg.pin_miso = -1; // MISO 핀 (사용 안함)
            cfg.pin_dc   = 10; // Data/Command 핀
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = 9;  // CS 핀
            cfg.pin_rst  = 14; // RST 핀
            cfg.pin_busy = -1; // Busy 핀 (사용 안함)
            cfg.panel_width  = 240; // 해상도 가로
            cfg.panel_height = 240; // 해상도 세로
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

// 디스플레이 인스턴스 생성
LGFX lcd;

/*
 * [문서화] app_main 함수
 * ESP-IDF의 시작점입니다. C 언어 스타일의 링킹을 위해 extern "C"를 사용합니다.
 */
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "ESP-IDF Environment Started!");

    // 1. 디스플레이 초기화
    if (lcd.init()) {
        ESP_LOGI(TAG, "LovyanGFX Initialized Successfully.");
    } else {
        ESP_LOGE(TAG, "LovyanGFX Initialization Failed!");
    }

    // 2. 화면 기본 설정 및 출력
    lcd.setBrightness(128); // 백라이트 밝기 (0~255)
    lcd.fillScreen(TFT_BLUE); // 배경을 파란색으로 채움
    lcd.setTextColor(TFT_WHITE); // 글자색을 흰색으로
    lcd.setTextSize(2); // 글자 크기 지정
    
    // 화면 중앙에 텍스트 출력
    lcd.setCursor(10, 10);
    lcd.printf("Hello, ESP-IDF!");

    // 3. 메인 루프 (FreeRTOS 태스크)
    int count = 0;
    while (true) {
        lcd.fillRect(10, 50, 200, 30, TFT_BLUE); // 이전 텍스트 덮어쓰기 (지우기)
        lcd.setCursor(10, 50);
        lcd.printf("Count: %d", count++);
        
        ESP_LOGI(TAG, "Running Loop... Count: %d", count);
        
        // 1000ms(1초) 대기
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}