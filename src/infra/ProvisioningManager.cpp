#include "infra/ProvisioningManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_sntp.h>
#include <cstdio>
#include <cstring>
#include <ctime>

#if CONFIG_BT_BLUEDROID_ENABLED
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#endif

namespace incubator::infra
{
namespace
{
    const char* TAG = "Provisioning";
    ProvisioningManager* g_instance = nullptr;

#if CONFIG_BLUEDROID_ENABLED
    void provisioningEvent(arduino_event_t* event)
    {
        if (g_instance && event) {
            g_instance->handleEvent(static_cast<int32_t>(event->event_id), &event->event_info);
        }
    }
#endif
}

void ProvisioningManager::init()
{
    buildDeviceIdentity();
    g_instance = this;
#if CONFIG_BLUEDROID_ENABLED
    WiFi.onEvent(provisioningEvent);
#endif

    WiFi.mode(WIFI_STA);
    if (m_settings.wifiConfigured) {
        WiFi.begin();
        m_state = ProvisioningState::Idle;
        startTimeSync();
    }
}

void ProvisioningManager::startBootProvisioning(uint32_t nowMs)
{
    if (m_settings.wifiConfigured) return;
    start(nowMs, m_settings.bootProvisionTimeoutMs, true);
}

void ProvisioningManager::requestMenuProvisioning(uint32_t nowMs)
{
    start(nowMs, m_settings.menuProvisionTimeoutMs, true);
}

void ProvisioningManager::start(uint32_t nowMs, uint32_t timeoutMs, bool resetProvisioned)
{
    buildDeviceIdentity();
    m_startedAtMs = nowMs;
    m_timeoutMs = timeoutMs;
    if (m_active) {
        stopProvisioning();
    }

    m_active = true;
    m_startedStack = false;
    m_finishAtMs = 0;
    m_state = ProvisioningState::Advertising;

#if CONFIG_BLUEDROID_ENABLED
    ESP_LOGI(TAG, "Starting BLE provisioning: name=%s pop=%s timeout=%ums",
             m_deviceName, m_pop, static_cast<unsigned>(timeoutMs));
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_STA);

    static uint8_t uuid[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                               0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02};
    wifi_prov_mgr_config_t config = {};
    config.scheme = wifi_prov_scheme_ble;
    config.scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM;
    config.app_event_handler.event_cb = nullptr;
    config.app_event_handler.user_data = nullptr;

    esp_err_t err = wifi_prov_mgr_init(config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_prov_mgr_init failed: %s", esp_err_to_name(err));
        m_state = ProvisioningState::Failed;
        m_active = false;
        return;
    }

    if (resetProvisioned) {
        err = wifi_prov_mgr_reset_provisioning();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "wifi_prov_mgr_reset_provisioning failed: %s", esp_err_to_name(err));
        }
    }

    err = wifi_prov_scheme_ble_set_service_uuid(uuid);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "wifi_prov_scheme_ble_set_service_uuid failed: %s", esp_err_to_name(err));
    }

    err = wifi_prov_mgr_disable_auto_stop(1000U);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "wifi_prov_mgr_disable_auto_stop failed: %s", esp_err_to_name(err));
    }

    err = wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, m_pop, m_deviceName, nullptr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_prov_mgr_start_provisioning failed: %s", esp_err_to_name(err));
        wifi_prov_mgr_deinit();
        m_state = ProvisioningState::Failed;
        m_active = false;
        return;
    }
    m_startedStack = true;
#else
    (void)resetProvisioned;
    ESP_LOGE(TAG, "BLE provisioning requested, but CONFIG_BLUEDROID_ENABLED is disabled");
    m_state = ProvisioningState::Failed;
    m_active = false;
#endif
}

void ProvisioningManager::cancel()
{
    if (!m_active) return;
    stopProvisioning();
    m_active = false;
    m_state = ProvisioningState::LocalMode;
    ESP_LOGI(TAG, "Provisioning cancelled; local mode");
}

void ProvisioningManager::tick(uint32_t nowMs)
{
if (WiFi.status() == WL_CONNECTED) {
        markConnected();
        if (m_active && m_state == ProvisioningState::Succeeded &&
            m_finishAtMs > 0U && nowMs >= m_finishAtMs) {
            
            stopProvisioning();
            m_active = false;

            // [추가] 프로비저닝이 성공적으로 끝난 직후, 새 설정을 클린 로드하기 위해 시스템을 재부팅합니다.
            ESP_LOGI(TAG, "▶▶▶ 프로비저닝 성공! 새 네트워크 환경 적용을 위해 시스템을 재부팅합니다. ◀◀◀");
            delay(500); // 로그가 시리얼 버퍼에 완전히 출력될 수 있도록 잠시 대기
            esp_restart(); // 🚀 ESP32 강제 칩 리셋
        }
        return;
    }
    
    if (!m_active) {
        if (m_settings.wifiConfigured && nowMs - m_lastConnectLogMs > 10000U) {
            m_lastConnectLogMs = nowMs;
            ESP_LOGW(TAG, "WiFi configured but not connected; waiting for STA reconnect");
            WiFi.reconnect();
        }
        return;
    }

    if (m_timeoutMs > 0U && nowMs - m_startedAtMs >= m_timeoutMs) {
        if (m_state == ProvisioningState::Saving) {
            m_startedAtMs = nowMs;
            m_timeoutMs = 60000U;
            ESP_LOGW(TAG, "Still waiting for WiFi IP after provisioning");
            return;
        }
        stopProvisioning();
        m_active = false;
        m_state = ProvisioningState::TimedOut;
        ESP_LOGW(TAG, "Provisioning timed out; local mode");
    }
}

bool ProvisioningManager::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

uint32_t ProvisioningManager::remainingMs(uint32_t nowMs) const
{
    if (!m_active || m_timeoutMs == 0U) return 0U;
    if (m_state == ProvisioningState::Succeeded) return 0U;
    uint32_t elapsed = nowMs - m_startedAtMs;
    if (elapsed >= m_timeoutMs) return 0U;
    return m_timeoutMs - elapsed;
}

const char* ProvisioningManager::statusText() const
{
    switch (m_state) {
        case ProvisioningState::Advertising: return "Waiting";
        case ProvisioningState::Saving: return "Saving";
        case ProvisioningState::Succeeded: return "Connected";
        case ProvisioningState::TimedOut: return "Timed out";
        case ProvisioningState::LocalMode: return "Local mode";
        case ProvisioningState::Failed: return "Setup failed";
        default: return "Ready";
    }
}

void ProvisioningManager::stopProvisioning()
{
#if CONFIG_BLUEDROID_ENABLED
    if (m_startedStack) {
        wifi_prov_mgr_stop_provisioning();
        wifi_prov_mgr_deinit();
        m_startedStack = false;
        m_finishAtMs = 0;
    }
#endif
}

void ProvisioningManager::buildDeviceIdentity()
{
    if (m_deviceName[0] != '\0') return;

    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    std::snprintf(m_deviceName, sizeof(m_deviceName), "PROV_%02X%02X%02X",
                  mac[3], mac[4], mac[5]);
    std::snprintf(m_pop, sizeof(m_pop), "%02X%02X%02X%02X",
                  mac[2], mac[3], mac[4], mac[5]);
}

void ProvisioningManager::markConnected()
{
    // [수정] 프로비저닝 동작 수행 중(m_active)에 아이피를 할당받았다면 
    // handleEvent에서 확보한 SSID와 Password가 포함된 m_settings 블록을 NVS에 온전하게 영구 저장합니다.
    if (m_active) {
        m_settings.wifiConfigured = true;
        m_nvs.saveBlob(storage::NvsStorage::kKeySettings, &m_settings, sizeof(m_settings));
        ESP_LOGI(TAG, "New Wi-Fi credentials committed to NVS Storage successfully.");
    }

    if (m_active || m_state != ProvisioningState::Succeeded) {
        m_state = ProvisioningState::Succeeded;
        if (m_active && m_finishAtMs == 0U) {
            m_finishAtMs = millis() + 2000U; 
        } else if (!m_active) {
            stopProvisioning();
        }
        startTimeSync();
        ESP_LOGI(TAG, "WiFi connected: %s", WiFi.localIP().toString().c_str());
    }
}

void ProvisioningManager::handleEvent(int32_t eventId, void* eventInfo)
{
#if CONFIG_BLUEDROID_ENABLED
    auto* info = static_cast<arduino_event_info_t*>(eventInfo);
    switch (eventId) {
        case ARDUINO_EVENT_PROV_START:
            m_active = true;
            m_state = ProvisioningState::Advertising;
            ESP_LOGI(TAG, "Provisioning started");
            break;

        case ARDUINO_EVENT_PROV_CRED_RECV:
            m_state = ProvisioningState::Saving;
            m_startedAtMs = millis();
            m_timeoutMs = 60000U;
            ESP_LOGI(TAG, "Provisioning credentials received: ssid=%s",
                     reinterpret_cast<const char*>(info->prov_cred_recv.ssid));

            // [수정] 수신된 옥텟 스트림 자격 증명에서 SSID와 Password를 안전하게 적출하여 구조체에 선제 기록합니다.
            std::memset(m_settings.wifiSsid, 0, sizeof(m_settings.wifiSsid));
            std::strncpy(m_settings.wifiSsid, reinterpret_cast<const char*>(info->prov_cred_recv.ssid), sizeof(m_settings.wifiSsid) - 1U);
            
            std::memset(m_settings.wifiPassword, 0, sizeof(m_settings.wifiPassword));
            std::strncpy(m_settings.wifiPassword, reinterpret_cast<const char*>(info->prov_cred_recv.password), sizeof(m_settings.wifiPassword) - 1U);
            break;

        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            m_state = ProvisioningState::Saving;
            m_startedAtMs = millis();
            m_timeoutMs = 60000U;
            ESP_LOGI(TAG, "Provisioning credentials accepted");
            break;

        case ARDUINO_EVENT_PROV_CRED_FAIL:
            m_state = ProvisioningState::Failed;
            m_startedAtMs = millis();
            m_timeoutMs = 60000U;
            m_settings.wifiConfigured = false;
            std::memset(m_settings.wifiSsid, 0, sizeof(m_settings.wifiSsid));
            std::memset(m_settings.wifiPassword, 0, sizeof(m_settings.wifiPassword));
            m_nvs.saveBlob(storage::NvsStorage::kKeySettings, &m_settings, sizeof(m_settings));
            {
                esp_err_t resetErr = wifi_prov_mgr_reset_sm_state_on_failure();
                if (resetErr != ESP_OK) {
                    ESP_LOGW(TAG, "Provisioning failure reset failed: %s", esp_err_to_name(resetErr));
                }
            }
            ESP_LOGW(TAG, "Provisioning credentials failed: reason=%d",
                     static_cast<int>(info->prov_fail_reason));
            break;

        case ARDUINO_EVENT_PROV_END:
            ESP_LOGI(TAG, "Provisioning ended");
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            markConnected();
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (m_settings.wifiConfigured && !m_active) {
                WiFi.reconnect();
            }
            break;

        default:
            break;
    }
#else
    (void)eventId;
    (void)eventInfo;
#endif
}

void ProvisioningManager::startTimeSync()
{
    if (m_timeSyncStarted) return;
    setenv("TZ", "KST-9", 1);
    tzset();
    if (esp_sntp_enabled()) {
        m_timeSyncStarted = true;
        return;
    }
    esp_sntp_setoperatingmode(static_cast<esp_sntp_operatingmode_t>(SNTP_OPMODE_POLL));
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_init();
    m_timeSyncStarted = true;
    ESP_LOGI(TAG, "SNTP time sync started");
}

} // namespace incubator::infra
