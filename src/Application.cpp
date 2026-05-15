#include "Application.h"
#include <nvs_flash.h>
#include <esp_task_wdt.h>
#include <esp_sntp.h>
#include <Arduino.h>
#include <esp_log.h>

static const char* TAG = "App";

namespace incubator
{

// 초기화 리스트를 통해 의존성을 주입(Combine)합니다.
Application::Application()
    : m_aht20(m_i2c),
      m_heater(static_cast<gpio_num_t>(config::Pin::SSR_HEATER)),
      m_humidifier(static_cast<gpio_num_t>(config::Pin::SSR_HUMIDIFIER)),
      m_turner(static_cast<gpio_num_t>(config::Pin::RELAY_TURNER)),
      m_buzzer(static_cast<gpio_num_t>(config::Pin::BUZZER)),
      m_fan(static_cast<int>(config::Pin::FAN_PWM), LEDC_CHANNEL_0),
      m_encoder(config::Pin::ENC_A, config::Pin::ENC_B, config::Pin::ENC_BTN),
      
      m_settings(domain::AppSettings::defaults()),
      m_state(domain::RuntimeState::zero()),
      
      m_sensorMgr(m_aht20, m_state),
      m_scheduler(m_state, m_batch, m_plan),
      m_climate(m_state, m_settings, m_heater, m_humidifier, m_buzzer),
      m_turning(m_state, m_settings, m_plan, m_turner),
      
      m_appCtrl(m_state, m_settings, m_batch, m_plan, m_nvs, m_planStorage,
                m_heater, m_humidifier, m_turner, m_fan),
      m_provisioning(m_settings, m_nvs),
      
      m_uiCtrl(m_uiModel, m_state, m_plan, m_appCtrl, m_provisioning, m_encoder),
      m_renderer(m_uiModel, m_display)
{
}

void Application::init()
{
    ESP_LOGI(TAG, "=== Incubator FW %s boot ===", INCUBATOR_FW_VERSION);

    if (!m_nvs.init()) {
        ESP_LOGE(TAG, "NVS init failed — halting");
        while (true) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    m_appCtrl.restoreFromStorage();
    m_provisioning.init();

    if (!m_i2c.init(config::Pin::I2C_SDA, config::Pin::I2C_SCL, 400000U)) {
        ESP_LOGE(TAG, "I2C init failed");
    }

    if (!m_aht20.init()) {
        ESP_LOGW(TAG, "AHT20 init failed — sensor fault mode");
    }

    ESP_LOGI(TAG, "Init outputs");
    m_heater.init();
    m_humidifier.init();
    m_turner.init();
    m_buzzer.init();
    m_fan.init();
    ESP_LOGI(TAG, "Outputs initialized");

    ESP_LOGI(TAG, "Init display");
    if (!m_display.init()) {
        ESP_LOGE(TAG, "Display init failed");
    }
    ESP_LOGI(TAG, "Display initialized");

    ESP_LOGI(TAG, "Init encoder");
    m_encoder.init();
    ESP_LOGI(TAG, "Encoder initialized");

    if (!m_planStorage.init()) {
        ESP_LOGE(TAG, "SPIFFS init failed");
    }

    m_appCtrl.validateAndRepairPlan();

#ifdef INCUBATOR_ENABLE_CLOUD
    m_wifiMgr.init(WIFI_SSID, WIFI_PASSWORD);

    // 변경됨: 더 이상 하드코딩된 byte 배열을 사용하지 않으므로 extern 선언 삭제
    // 변경됨: 새 AwsIotClient::init() 시그니처에 맞게 m_planStorage 전달
    if (!m_awsClient.init(AWS_IOT_ENDPOINT, INCUBATOR_DEVICE_ID, m_planStorage)) {
        ESP_LOGE(TAG, "AWS IoT Client init failed (Check LittleFS certs)");
    }

    // 람다 함수에서 this 포인터를 캡처하여 내부 m_appCtrl 사용
    m_awsClient.setCmdCallback(
        [this](const char* topic, const char* payload) {
            cloud::CmdParser::parse(payload, this->m_appCtrl);
        }
    );

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
#endif

    // Watchdog 설정 (이전 에러 해결 반영)
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = kWatchdogTimeoutMs,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, 
        .trigger_panic = true                            
    };

    esp_err_t wdtErr = esp_task_wdt_init(&wdt_config);
    if (wdtErr != ESP_OK && wdtErr != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "WDT init failed: %s", esp_err_to_name(wdtErr));
    }
    esp_task_wdt_add(nullptr); 

    // 부팅 화면 렌더링
    {
        auto& gfx = m_display.gfx();
        gfx.startWrite();
        gfx.fillScreen(0x0000);
        gfx.setTextColor(0xFFFF, 0x0000);
        gfx.setTextSize(2);
        gfx.setCursor(40, 90);
        gfx.print("INCUBATOR");
        gfx.setTextSize(1);
        gfx.setCursor(80, 120);
        gfx.print(INCUBATOR_FW_VERSION);
        gfx.setCursor(50, 140);
        gfx.print("Initializing...");
        gfx.endWrite();
        delay(800);
    }

    m_provisioning.startBootProvisioning(millis());

    ESP_LOGI(TAG, "Setup complete. Boot#%u", m_state.bootCount);
}

void Application::tick()
{
    uint32_t now = millis();
    
    m_sensorMgr.tick(now);
    m_scheduler.tick(now);
    m_climate.tick(now);
    m_turning.tick(now);
    m_provisioning.tick(now);
    m_encoder.tick(now);
    m_uiCtrl.tick(now);
    m_renderer.render(now);

#ifdef INCUBATOR_ENABLE_CLOUD
    m_wifiMgr.tick(now);
    m_awsClient.tick(now);
#endif
}

} // namespace incubator