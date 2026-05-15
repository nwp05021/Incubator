#ifdef INCUBATOR_ENABLE_CLOUD
#include "cloud/AwsIotClient.h"
#include "storage/PlanStorage.h" // PlanStorage 인터페이스 포함
#include <esp_log.h>\n#include <cstring>
#include <cstdio>

namespace incubator::cloud
{
namespace
{
    const char* TAG = "AwsIotClient";

    // data/certs 폴더는 LittleFS 루트 마운트 접두어인 /littlefs 아래인 certs에 안착됩니다.
    const char* FILE_ROOT_CA   = "/littlefs/certs/aws-root-ca.pem";
    const char* FILE_CERT      = "/littlefs/certs/certificate.pem.crt";
    const char* FILE_KEY       = "/littlefs/certs/private.pem.key";

    bool copyChunk(char* dst, size_t dstSize, int offset, const char* src, int len)
    {
        if (!dst || !src || offset < 0 || len < 0) return false;
        if (static_cast<size_t>(offset + len) >= dstSize) return false;
        std::memcpy(dst + offset, src, static_cast<size_t>(len));
        dst[offset + len] = '\0';
        return true;
    }
}

bool AwsIotClient::init(const char* endpoint, const char* deviceId, storage::PlanStorage& storage)
{
    if (!endpoint || !deviceId) {
        ESP_LOGE(TAG, "Missing AWS IoT configuration parameters");
        return false;
    }

    // 1. 연동 검증: PlanStorage가 사전에 LittleFS 마운트를 끝마쳤는지 체크
    if (!storage.isInitialized()) {
        ESP_LOGE(TAG, "PlanStorage(LittleFS)가 마운트되지 않아 인증서를 읽을 수 없습니다.");
        return false;
    }

    // 2. LittleFS 경로에서 파일 데이터를 주입받아 std::string 멤버 변수에 보관
    if (!readFileToString(FILE_ROOT_CA, m_rootCaPemStr)) {
        ESP_LOGE(TAG, "Root CA 인증서 파일 로드 실패: %s", FILE_ROOT_CA);
        return false;
    }
    if (!readFileToString(FILE_CERT, m_certPemStr)) {
        ESP_LOGE(TAG, "Device Certificate 파일 로드 실패: %s", FILE_CERT);
        return false;
    }
    if (!readFileToString(FILE_KEY, m_keyPemStr)) {
        ESP_LOGE(TAG, "Private Key 파일 로드 실패: %s", FILE_KEY);
        return false;
    }

    std::strncpy(m_deviceId, deviceId, sizeof(m_deviceId) - 1);
    std::snprintf(m_uri, sizeof(m_uri), "mqtts://%s:%d", endpoint, kMqttPort);
    std::snprintf(m_cmdTopic, sizeof(m_cmdTopic), "incubator/%s/cmd", m_deviceId);
    std::snprintf(m_telemetryTopic, sizeof(m_telemetryTopic), "incubator/%s/telemetry", m_deviceId);
    std::snprintf(m_healthTopic, sizeof(m_healthTopic), "incubator/%s/health", m_deviceId);

    esp_mqtt_client_config_t mqttCfg = {};
    mqttCfg.broker.address.uri = m_uri;
    // 읽어온 string 버퍼 데이터를 AWS IoT TLS 설정에 바인딩
    mqttCfg.broker.verification.certificate = m_rootCaPemStr.c_str();
    mqttCfg.credentials.authentication.certificate = m_certPemStr.c_str();
    mqttCfg.credentials.authentication.key = m_keyPemStr.c_str();
    mqttCfg.credentials.client_id = m_deviceId;

    m_client = esp_mqtt_client_init(&mqttCfg);
    if (!m_client) {
        ESP_LOGE(TAG, "Failed to initialize esp_mqtt_client");
        return false;
    }

    esp_mqtt_client_register_event(m_client, MQTT_EVENT_ANY, mqttEventHandler, this);
    esp_err_t err = esp_mqtt_client_start(m_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start esp_mqtt_client: %d", err);
        return false;
    }

    ESP_LOGI(TAG, "AWS IoT Client가 LittleFS 인증서를 장착하고 정상 시작되었습니다.");
    return true;
}

bool AwsIotClient::readFileToString(const char* filepath, std::string& output)
{
    FILE* f = std::fopen(filepath, "r");
    if (!f) {
        return false;
    }

    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    if (size < 0) {
        std::fclose(f);
        return false;
    }
    std::fseek(f, 0, SEEK_SET);

    output.resize(static_cast<size_t>(size));
    size_t readBytes = std::fread(&output[0], 1, static_cast<size_t>(size), f);
    std::fclose(f);

    if (readBytes < static_cast<size_t>(size)) {
        output.resize(readBytes);
    }
    return true;
}

// ...이하 tick(), publish() 및 이벤트 핸들러 로직은 기존 소스와 동일...
void AwsIotClient::handleMqttEvent(int32_t eventId, void* eventData)
{
    auto* event = static_cast<esp_mqtt_event_handle_t>(eventData);
    if (!event) return;

    switch (static_cast<esp_mqtt_event_id_t>(eventId)) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            m_connected = true;
            esp_mqtt_client_subscribe(m_client, m_cmdTopic, kMqttQos);
            publishOnlineStatus();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            m_connected = false;
            break;
        case MQTT_EVENT_DATA:
            if (event->topic && event->topic_len > 0) {
                m_rxTotalLen = event->data_len;
                if (static_cast<size_t>(event->topic_len) >= sizeof(m_rxTopic) ||
                    static_cast<size_t>(m_rxTotalLen) >= sizeof(m_rxPayload)) {
                    ESP_LOGW(TAG, "Dropping oversized MQTT message");
                    m_rxTotalLen = 0;
                    return;
                }
                copyChunk(m_rxTopic, sizeof(m_rxTopic), 0, event->topic, event->topic_len);
            }
            if (m_rxTotalLen <= 0) return;
            if (!copyChunk(m_rxPayload, sizeof(m_rxPayload), event->current_data_offset, event->data, event->data_len)) {
                ESP_LOGW(TAG, "Dropping fragmented MQTT message");
                m_rxTotalLen = 0;
                return;
            }
            if (event->current_data_offset + event->data_len >= m_rxTotalLen) {
                if (m_cmdCb) {
                    m_cmdCb(m_rxTopic, m_rxPayload);
                }
                m_rxTotalLen = 0;
            }
            break;
        default:
            break;
    }
}

void AwsIotClient::mqttEventHandler(void* handlerArgs, esp_event_base_t, int32_t eventId, void* eventData)
{
    auto* client = static_cast<AwsIotClient*>(handlerArgs);
    if (client) {
        client->handleMqttEvent(eventId, eventData);
    }
}

void AwsIotClient::tick(uint32_t) {}

bool AwsIotClient::publish(const char* topic, const char* json)
{
    if (!m_client || !m_connected || !topic || !json) return false;
    return esp_mqtt_client_publish(m_client, topic, json, 0, kMqttQos, 0) >= 0;
}

bool AwsIotClient::publishTelemetry(const char* json)
{
    return publish(m_telemetryTopic, json);
}

bool AwsIotClient::publishHealth(const char* json, bool retain)
{
    if (!m_client || !m_connected || !json) return false;
    return esp_mqtt_client_publish(m_client, m_healthTopic, json, 0, kMqttQos, retain ? 1 : 0) >= 0;
}

} // namespace incubator::cloud
#endif