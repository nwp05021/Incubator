#ifdef INCUBATOR_ENABLE_CLOUD
#include "cloud/WifiManager.h"
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <lwip/ip4_addr.h>
#include <cstring>
#include <cstdio>

static const char* TAG = "WifiManager";

namespace incubator::cloud
{

WifiManager* WifiManager::s_instance = nullptr;

bool WifiManager::init(const char* ssid, const char* password)
{
    if (!ssid || ssid[0] == '\0') {
        ESP_LOGE(TAG, "Missing WiFi SSID");
        return false;
    }

    m_ssid = ssid;
    m_password = password ? password : "";
    s_instance = this;

    // 1. 이미 초기화되었는지 확인 (중복 호출 방지)
    if (esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") != nullptr) {
        ESP_LOGW(TAG, "WiFi STA Netif already exists. Skipping init.");
    } else {
        // 2. 필수 인프라 초기화 (실패해도 이미 된 경우라면 무시)
        esp_netif_init(); 
        esp_event_loop_create_default();

        // 3. STA 인터페이스 생성 (최초 1회만)
        esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
        if (sta_netif == nullptr) {
            ESP_LOGE(TAG, "Failed to create default WiFi STA netif");
            return false;
        }
    }
    
    wifi_init_config_t initCfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&initCfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::eventHandler, this, nullptr);
    esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::eventHandler, this, nullptr);

    wifi_config_t wifiCfg = {};
    std::strncpy(reinterpret_cast<char*>(wifiCfg.sta.ssid), m_ssid, sizeof(wifiCfg.sta.ssid) - 1U);
    std::strncpy(reinterpret_cast<char*>(wifiCfg.sta.password), m_password, sizeof(wifiCfg.sta.password) - 1U);
    wifiCfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifiCfg.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiCfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

    m_lastRetryMs = 0;
    m_connected = false;
    return true;
}

void WifiManager::eventHandler(void* arg,
                               esp_event_base_t eventBase,
                               int32_t eventId,
                               void* eventData)
{
    auto* self = static_cast<WifiManager*>(arg ? arg : s_instance);
    if (self) self->handleEvent(eventBase, eventId, eventData);
}

void WifiManager::handleEvent(esp_event_base_t eventBase, int32_t eventId, void* eventData)
{
    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
        m_connected = false;
        m_ipAddress[0] = '\0';
        ESP_LOGW(TAG, "WiFi disconnected");
        return;
    }

    if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(eventData);
        std::snprintf(m_ipAddress, sizeof(m_ipAddress), IPSTR, IP2STR(&event->ip_info.ip));
        m_connected = true;
        ESP_LOGI(TAG, "WiFi connected: %s", m_ipAddress);
    }
}

void WifiManager::tick(uint32_t nowMs)
{
    if (m_connected) return;

    if (nowMs - m_lastRetryMs < kRetryIntervalMs) {
        return;
    }

    m_lastRetryMs = nowMs;
    esp_wifi_connect();
    ESP_LOGI(TAG, "Retrying WiFi");
}

bool WifiManager::isConnected() const
{
    return m_connected;
}

const char* WifiManager::ipAddress() const
{
    return m_connected ? m_ipAddress : "";
}

} // namespace incubator::cloud
#endif
