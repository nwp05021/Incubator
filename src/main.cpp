#include "Application.h"
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// 애플리케이션 싱글톤 인스턴스 생성
incubator::Application app;

void setup()
{
    Serial.begin(115200);
    app.init();
}

void loop()
{
    app.tick();
    esp_task_wdt_reset();
}

extern "C" void app_main()
{
    initArduino();
    setup();
    while (true) {
        loop();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}