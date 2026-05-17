#ifdef INCUBATOR_ENABLE_CLOUD
#include "cloud/AwsIotClient.h"
#include "storage/PlanStorage.h" 
#include <esp_log.h>
#include <cstring>
#include <cstdio>
#include <ctime> // <-- [추가] time_t 및 tm 구조체 사용을 위해 이 줄을 추가합니다.

static const char* APP_TAG = "BasicIngestApp";

namespace incubator::cloud
{
namespace
{
    const char* TAG = "AwsIotClient";

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

    // --- [디버깅 코드 추가] 주소 앞뒤 공백이나 프로토콜 중복 여부 확인 ---
    ESP_LOGI(TAG, "▶▶▶ 현재 로드된 AWS 엔드포인트 주소: [%s] ◀◀◀", endpoint);
    
    if (!storage.isInitialized()) {
        ESP_LOGE(TAG, "PlanStorage(LittleFS)가 마운트되지 않아 인증서를 읽을 수 없습니다.");
        return false;
    }

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
    
    // [변경] AWS IoT 규칙 주제(esp32/+/telemetry) 일치를 위해 토픽 접두어를 esp32로 변경
    std::snprintf(m_telemetryTopic, sizeof(m_telemetryTopic), "esp32/%s/telemetry", m_deviceId);
    std::snprintf(m_healthTopic, sizeof(m_healthTopic), "incubator/%s/health", m_deviceId);

    esp_mqtt_client_config_t mqttCfg = {};
    mqttCfg.broker.address.uri = m_uri;
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
    if (!f) return false;

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

void AwsIotClient::handleMqttEvent(int32_t eventId, void* eventData)
{
    auto* event = static_cast<esp_mqtt_event_handle_t>(eventData);
    if (!event) return;

    switch (static_cast<esp_mqtt_event_id_t>(eventId)) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            m_connected = true;
            esp_mqtt_client_subscribe(m_client, m_cmdTopic, kMqttQos);
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

/**
 * @brief 요약 설명: 5초 간격 데이터 수집 및 50초 주기 일괄 전송 로직
 * 1. MQTT 브로커와 연결된 상태(`m_connected`)에서만 샘플링 타이머가 동작합니다.
 * 2. 5000ms(5초)가 경과할 때마다 입력된 온습도 데이터를 배열(`m_tempSamples`, `m_humidSamples`)에 순차 저장합니다.
 * 3. 수집 카운트(`m_sampleCount`)가 10개에 도달하면(총 50초), 데이터를 하나의 JSON 배열 구조로 직렬화합니다.
 * 4. 구성된 텔레메트리 토픽(esp32/Incubator-1001/telemetry)으로 Publish하여 AWS 규칙 및 Lambda를 트리거합니다.
 */
void AwsIotClient::tick(uint32_t now, float currentTemp, float currentHumidity) 
{
    // [디버깅 추가]: 연결되지 않은 상태에서 tick이 돌고 있는지 5초마다 확인 로그 출력
    static uint32_t lastDebugTime = 0;
    if (now - lastDebugTime >= 5000) {
        lastDebugTime = now;
        time_t nowSec = time(nullptr);
        struct tm timeinfo;
        localtime_r(&nowSec, &timeinfo);
        
        ESP_LOGI(TAG, "AwsIotClient::tick 실행 중... (MQTT 연결상태: %s, 현재 연도: %d)", 
                 m_connected ? "연결됨" : "미연결(대기중)", 
                 timeinfo.tm_year + 1900);
    }

    if (!m_connected) {
        // 연결이 끊긴 동안은 타이머를 현재 시간으로 동기화하여 재연결 시점부터 정밀하게 작동하도록 함
        m_lastSampleTime = now;
        return;
    }

    // 1단계: 5초(5000ms) 주기 체크하여 데이터 버퍼링
    if (now - m_lastSampleTime >= 5000) {
        m_lastSampleTime = now;

        if (m_sampleCount < 10) {
            m_tempSamples[m_sampleCount] = currentTemp;
            m_humidSamples[m_sampleCount] = currentHumidity;
            m_sampleCount++;
            ESP_LOGD(TAG, "센서 데이터 수집 중... (%d/10) - T: %.2f, H: %.2f", m_sampleCount, currentTemp, currentHumidity);
        }

        // 2단계: 10개 샘플링 완료 시 (50초 경과) 일괄 JSON 조립 및 전송
        if (m_sampleCount >= 10) {
            char jsonBuffer[1024]; // 10개 배치 데이터를 담기에 안전한 stack 버퍼 크기 할당
            int offset = std::snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"thingName\":\"%s\",\"samples\":[", m_deviceId);

            for (int i = 0; i < 10; ++i) {
                int written = std::snprintf(jsonBuffer + offset, sizeof(jsonBuffer) - offset,
                    "{\"t\":%.2f,\"h\":%.2f}%s",
                    m_tempSamples[i], m_humidSamples[i], (i == 9) ? "" : ",");
                if (written > 0) {
                    offset += written;
                }
            }
            std::snprintf(jsonBuffer + offset, sizeof(jsonBuffer) - offset, "]}");

            // 3단계: MQTT 전송 실행
            if (publishTelemetry(jsonBuffer)) {
                ESP_LOGI(TAG, "50초 주기 센서 배치 데이터 전송 성공 (Lambda 호출 트리거)");
            } else {
                ESP_LOGW(TAG, "배치 데이터 전송 실패");
            }

            // 카운터를 초기화하여 다음 50초 배치 데이터 수집 준비
            m_sampleCount = 0;
        }
    }
}

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