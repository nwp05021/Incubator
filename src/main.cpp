#include <Arduino.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// 리팩토링된 디스플레이 헤더 포함
#include "devices/St7789Display.h"

static const char *TAG = "MAIN";

// 디스플레이 객체 생성 (네임스페이스 활용)
using incubator::devices::St7789Display;
St7789Display display;

extern "C" void app_main(void)
{
    initArduino(); // Arduino 컴포넌트 초기화
    
    ESP_LOGI(TAG, "Refactored ESP-IDF Environment Started!");

    // 1. 디스플레이 초기화 (LgfxConfig.h에 정의된 설정 사용)
    if (display.init()) {
        ESP_LOGI(TAG, "St7789Display Initialized Successfully.");
    } else {
        ESP_LOGE(TAG, "St7789Display Initialization Failed!");
        return;
    }

    // 2. 초기 화면 설정
    // LovyanGFX의 직접 제어가 필요한 경우 display.gfx()를 통해 접근 가능합니다.
    display.gfx().setBrightness(128); 
    display.fillScreen(0x001F); // 파란색 (RGB565 포맷)
    display.setTextColor(0xFFFF); // 흰색
    display.setTextSize(2);

    display.drawText(10, 10, "Hello, Refactored!");

    // 3. 메인 루프
    int count = 0;
    char buf[32];

    while (true) {
        display.beginFrame(); // 프레임 시작 (Canvas/Sprite 사용 시 필요)

        // 이전 텍스트 영역 지우기
        display.fillRect(10, 50, 200, 30, 0x001F); 
        
        // 카운트 출력
        snprintf(buf, sizeof(buf), "Count: %d", count++);
        display.drawText(10, 50, buf);
        
        display.endFrame(); // 프레임 종료 및 화면 전송

        ESP_LOGI(TAG, "Running Loop... %s", buf);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}