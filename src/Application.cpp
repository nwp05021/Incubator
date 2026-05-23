#include "Application.h"
#include <nvs_flash.h>
#include <esp_task_wdt.h>
#include <esp_sntp.h>
#include <Arduino.h>
#include <esp_log.h>

static const char* TAG = "App";

namespace incubator
{

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
    if (m_settings.wifiConfigured) {
        ESP_LOGI(TAG, "저장된 Wi-Fi 설정을 감지했습니다. 일반 클라우드 모드로 진입합니다.");
        
        // 1. native Wi-Fi 매니저 가동
        m_wifiMgr.init(m_settings.wifiSsid, m_settings.wifiPassword);

        // 2. AWS IoT Client 인프라 생성 및 콜백 바인딩
        if (!m_awsClient.init(AWS_IOT_ENDPOINT, INCUBATOR_DEVICE_ID, m_planStorage)) {
            ESP_LOGE(TAG, "AWS IoT Client init failed (Check LittleFS certs)");
        }
        m_awsClient.setCmdCallback(
            [this](const char* topic, const char* payload) {
                cloud::CmdParser::parse(payload, this->m_appCtrl);
            }
        );
        m_uiCtrl.setAwsPublishCallback(
            [this](const char* topic, const char* payload) {
                return this->m_awsClient.publish(topic, payload);
            }
        );

        // ❌ [삭제/주석 처리] 이곳에 있던 esp_sntp_init() 관련 코드를 통째로 들어냅니다.
        // if (!esp_sntp_enabled()) { ... }
    }
#endif

    // Watchdog 설정
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

    // Wi-Fi가 미설정 상태라면 이 함수 내부에서 판단하여 즉시 BLE 어드버타이징을 시작합니다.
    m_provisioning.startBootProvisioning(millis());

    ESP_LOGI(TAG, "Setup complete. Boot#%u", (unsigned int)m_state.bootCount);    
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

    m_uiModel.provisioningActive = m_provisioning.isActive();
    m_uiModel.provisioningSucceeded = m_provisioning.isSucceeded();
    m_uiModel.provisioningFailed = m_provisioning.isFailed();
    m_uiModel.provisioningRemainingMs = m_provisioning.remainingMs(now);
    
    std::strncpy(m_uiModel.provisioningName, m_provisioning.deviceName(), sizeof(m_uiModel.provisioningName) - 1);
    std::strncpy(m_uiModel.provisioningPop, m_provisioning.proofOfPossession(), sizeof(m_uiModel.provisioningPop) - 1);
    std::strncpy(m_uiModel.provisioningMessage, m_provisioning.statusText(), sizeof(m_uiModel.provisioningMessage) - 1);

    m_renderer.render(now);

#ifdef INCUBATOR_ENABLE_CLOUD
    if (!m_provisioning.isActive() && m_settings.wifiConfigured) {
        m_wifiMgr.tick(now);

        // ─── [아키텍처 추가] 유효 IP 커넥션 확립 직후 SNTP 가동 ───
        static bool sntpReady = false;
        if (m_wifiMgr.isConnected()) {
            if (!sntpReady) {
                if (!esp_sntp_enabled()) {
                    ESP_LOGI(TAG, "▶ [네트워크 감지] 정상 IP 확보 완료. SNTP 인프라를 동적 활성화합니다.");
                    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
                    
                    // 국내 환경 응답 속도 극대화를 위한 멀티 레이어 타임 서버 배치
                    esp_sntp_setservername(0, "kr.pool.ntp.org");
                    esp_sntp_setservername(1, "time.windows.com");
                    esp_sntp_setservername(2, "pool.ntp.org");
                    
                    esp_sntp_init();

                    // 대한민국 표준시(KST) 타임존 환경변수 명시적 설정
                    setenv("TZ", "KST-9", 1);
                    tzset();
                    
                    ESP_LOGI(TAG, "▶ [SNTP] 타임 서버 바인딩 및 타임존(KST) 보정 완료.");
                }
                sntpReady = true;
            }
        } else {
            // 연결 유실 시 재연결 시점에 즉각 리프레시 쿼리를 날리고 싶다면
            // 이곳에서 sntpReady = false 처리를 고려할 수 있으나, lwIP 자체 폴링이 작동하므로
            // 부팅 직후 최초 동기화 지연 청소용으로는 이 true 유지 구조만으로 충분합니다.
        }

        m_awsClient.tick(now, m_state.currentTempC, m_state.currentHumidityPct);    
        m_state.cloudConnected = m_awsClient.isConnected();
    }
#endif
}

} // namespace incubator
//https://github.com/nwp05021/Incubator
