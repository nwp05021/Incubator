#ifdef INCUBATOR_ENABLE_CLOUD
#include "cloud/AwsIotClient.h"
#include "ui/UiModel.h" // ◀ 명세 배열(kAwsPublishEndpoints)을 참조하기 위해 추가
#include <esp_log.h>
#include <cstring>
#include <cstdio>
#include <ctime>

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

    std::strncpy(m_deviceId, deviceId, sizeof(m_deviceId) - 1);
    std::snprintf(m_uri, sizeof(m_uri), "mqtts://%s:%d", endpoint, kMqttPort);

    // [표준 토픽 세팅] 하드코딩을 걷어내고 UiModel의 헬퍼 함수를 통해 동적으로 포맷을 받아옵니다.
    const char* telemetryFmt = ui::findAwsTopicFormat("Telemetry");
    if (telemetryFmt) {
        std::snprintf(m_telemetryTopic, sizeof(m_telemetryTopic), telemetryFmt, m_deviceId);
    } else {
        m_telemetryTopic[0] = '\0'; // 안전하게 초기화  
        ESP_LOGW(TAG, "Telemetry topic format not found in UiModel!");
    }

    const char* cmdFmt = ui::findAwsTopicFormat("Cmd");
    if (cmdFmt) {
        std::snprintf(m_cmdTopic, sizeof(m_cmdTopic), cmdFmt, m_deviceId);
    } else {
        m_cmdTopic[0] = '\0'; // 안전하게 초기화
        ESP_LOGW(TAG, "Command topic format not found in UiModel!");
    }

    const char* shadowFmt = ui::findAwsTopicFormat("Shadow");
    if (shadowFmt) {
        std::snprintf(m_shadowUpdateTopic, sizeof(m_shadowUpdateTopic), shadowFmt, m_deviceId);
    } else {
        m_shadowUpdateTopic[0] = '\0'; // 안전하게 초기화
        ESP_LOGW(TAG, "Shadow topic format not found in UiModel!");
    }

    // Delta 토픽은 Shadow 포맷 규칙 기반으로 파생
    if (shadowFmt) {
        char shadowDeltaFmt[128];
        std::snprintf(shadowDeltaFmt, sizeof(shadowDeltaFmt), "%s/delta", shadowFmt);
        std::snprintf(m_shadowDeltaTopic, sizeof(m_shadowDeltaTopic), shadowDeltaFmt, m_deviceId);
    } else {
        m_shadowDeltaTopic[0] = '\0'; // 안전하게 초기화
        ESP_LOGW(TAG, "Shadow delta topic format not found!");
    }

    // 원본 인증서 로드 로직
    if (!readFileToString(FILE_ROOT_CA, m_rootCaPemStr) ||
        !readFileToString(FILE_CERT, m_certPemStr) ||
        !readFileToString(FILE_KEY, m_keyPemStr)) {
        ESP_LOGE(TAG, "Failed to load AWS IoT certificates from LittleFS");
        return false;
    }

    esp_mqtt_client_config_t mqttCfg = {};
    mqttCfg.broker.address.uri = m_uri;
    mqttCfg.credentials.client_id = m_deviceId;
    mqttCfg.credentials.authentication.certificate = m_certPemStr.c_str();
    mqttCfg.credentials.authentication.key = m_keyPemStr.c_str();
    mqttCfg.broker.verification.certificate = m_rootCaPemStr.c_str();

    m_client = esp_mqtt_client_init(&mqttCfg);
    if (!m_client) {
        ESP_LOGE(TAG, "Failed to initialize ESP MQTT client");
        return false;
    }

    esp_mqtt_client_register_event(m_client, MQTT_EVENT_ANY, &AwsIotClient::mqttEventHandler, this);
    esp_mqtt_client_start(m_client);

    return true;
}

// 💡 링커 에러 수정 1: 복원된 tick 함수 (10초 버퍼링 및 100초 주기 Batch 전송)
void AwsIotClient::tick(uint32_t now, float currentTemp, float currentHumidity)
{
    if (!m_connected) return;

    // 1️⃣ 10초(10000ms)마다 센서 데이터 샘플링 및 시간 기록
    if (m_lastSampleTime == 0 || (now - m_lastSampleTime >= 10000)) {
        m_lastSampleTime = now;

        if (m_sampleCount < 10) {
            m_tempSamples[m_sampleCount] = currentTemp;
            m_humidSamples[m_sampleCount] = currentHumidity;
            m_timeSamples[m_sampleCount]  = static_cast<uint32_t>(std::time(nullptr)); // 💡 샘플링 시점의 Epoch Time 기록
            m_sampleCount++;
        }
    }

    // 2️⃣ 10개의 샘플이 모두 모이면 AWS로 전송할 JSON 조립
    if (m_sampleCount >= 10) {
        char jsonBuffer[1024];
        // thingName을 포함한 기본 메타데이터 구조 시작
        int offset = std::snprintf(jsonBuffer, sizeof(jsonBuffer),
            "{\"thingName\":\"%s\",\"samples\":[", m_deviceId);

        // 개별 샘플 돌면서 't'(온도), 'h'(습도), 'ts'(타임스탬프) 구조로 조립
        for (int i = 0; i < 10; ++i) {
            int written = std::snprintf(jsonBuffer + offset, sizeof(jsonBuffer) - offset,
                "{\"t\":%.2f,\"h\":%.2f,\"ts\":%lu}%s", // ⚠️ %u를 %lu로 변경!
                m_tempSamples[i], m_humidSamples[i], m_timeSamples[i], 
                (i == 9) ? "" : ",");
            
            if (written > 0) {
                offset += written;
            }
        }

        // JSON 배열 및 객체 닫기
        std::snprintf(jsonBuffer + offset, sizeof(jsonBuffer) - offset, "]}");

        // MQTT Publish 실행
        if (publish(m_telemetryTopic, jsonBuffer)) {
            ESP_LOGI(TAG, "Successfully published 10 grouped telemetry data with timestamps");
        } else {
            ESP_LOGE(TAG, "Failed to publish telemetry data");
        }

        // 카운트 초기화하여 다음 100초 주기 준비
        m_sampleCount = 0;
    }
}

// 💡 링커 에러 수정 2: 복원된 기본 publish 함수
bool AwsIotClient::publish(const char* topic, const char* json)
{
    if (!m_client || !m_connected || !topic || !json) return false;
    
    int msgId = esp_mqtt_client_publish(m_client, topic, json, 0, kMqttQos, 0);
    if (msgId < 0) {
        ESP_LOGE(TAG, "Failed to publish message to topic: %s", topic);
        return false;
    }
    return true;
}

// 💡 추가된 디바이스 섀도우 상태 업데이트용 함수
bool AwsIotClient::reportTargetTemperature(float targetTemp)
{
    if (!m_client || !m_connected) return false;
    char shadowBuffer[256];
    std::snprintf(shadowBuffer, sizeof(shadowBuffer), "{\"state\":{\"reported\":{\"target_temp\":%.1f}}}", targetTemp);
    return publish(m_shadowUpdateTopic, shadowBuffer);
}

void AwsIotClient::handleMqttEvent(int32_t eventId, void* eventData)
{
    auto* event = static_cast<esp_mqtt_event_handle_t>(eventData);
    if (!event) return;

    switch (static_cast<esp_mqtt_event_id_t>(eventId)) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            m_connected = true;
            // 앱 제어용 섀도우와 재부팅용 일반 커맨드 토픽 구독
            esp_mqtt_client_subscribe(m_client, m_cmdTopic, kMqttQos);
            esp_mqtt_client_subscribe(m_client, m_shadowDeltaTopic, kMqttQos);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            m_connected = false;
            break;

        case MQTT_EVENT_DATA:
            if (event->current_data_offset == 0) {
                m_rxTotalLen = event->total_data_len;
                copyChunk(m_rxTopic, sizeof(m_rxTopic), 0, event->topic, event->topic_len);
                copyChunk(m_rxPayload, sizeof(m_rxPayload), 0, event->data, event->data_len);
            } else {
                copyChunk(m_rxPayload, sizeof(m_rxPayload), event->current_data_offset, event->data, event->data_len);
            }

            if (event->current_data_offset + event->data_len >= m_rxTotalLen) {
                // [경로 1]: 주기적 원격 재부팅 명령 체크
                if (std::strcmp(m_rxTopic, m_cmdTopic) == 0) {
                    if (std::strstr(m_rxPayload, "\"action\":\"REBOOT\"") != nullptr) {
                        ESP_LOGE(TAG, "안정성을 위해 기기를 소프트 리셋합니다.");
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        esp_restart(); 
                    } else if (m_cmdCb) {
                        m_cmdCb(m_rxTopic, m_rxPayload); 
                    }
                }
                // [경로 2]: 디바이스 섀도우 온도 조절 제어 수신
                else if (std::strcmp(m_rxTopic, m_shadowDeltaTopic) == 0) {
                    if (m_cmdCb) {
                        m_cmdCb(m_rxTopic, m_rxPayload); 
                    }
                }
                m_rxTotalLen = 0;
            }
            break;
        default:
            break;
    }
}

// 💡 링커 에러 수정 3: 복원된 파일 읽기 함수
bool AwsIotClient::readFileToString(const char* path, std::string& outStr)
{
    if (!path) return false;

    std::FILE* f = std::fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return false;
    }

    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    if (size < 0) {
        std::fclose(f);
        return false;
    }

    outStr.resize(static_cast<size_t>(size));
    size_t readBytes = std::fread(&outStr[0], 1, static_cast<size_t>(size), f);
    std::fclose(f);

    if (readBytes == 0 && size > 0) {
        ESP_LOGE(TAG, "Failed to read file contents: %s", path);
        return false;
    }

    outStr.resize(readBytes);
    return true;
}

void AwsIotClient::mqttEventHandler(void* handlerArgs, esp_event_base_t base, int32_t eventId, void* eventData)
{
    auto* client = static_cast<AwsIotClient*>(handlerArgs);
    if (client) {
        client->handleMqttEvent(eventId, eventData);
    }
}

} // namespace incubator::cloud
#endif