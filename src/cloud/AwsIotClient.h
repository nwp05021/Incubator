#pragma once
#include "storage/PlanStorage.h"
#include <functional>
#include <cstdint>
#include <string>

#ifdef INCUBATOR_ENABLE_CLOUD
#include <mqtt_client.h>
#endif

namespace incubator::cloud
{
    using CmdCallback = std::function<void(const char* topic, const char* payload)>;

    class AwsIotClient
    {
    public:
        static constexpr int      kMqttPort = 8883;
        static constexpr int      kMqttQos = 1;

        bool init(const char* endpoint, const char* deviceId, storage::PlanStorage& storage);
        void tick(uint32_t now, float currentTemp, float currentHumidity);
        bool publish(const char* topic, const char* json);
        bool isConnected() const { return m_connected; };
        void setCmdCallback(CmdCallback cb) { m_cmdCb = cb; }
        
        // 💡 디바이스 섀도우 전용 상태 보고 함수 추가
        bool reportTargetTemperature(float targetTemp);

        const char* telemetryTopic() const { return m_telemetryTopic; }
        const char* commandTopic() const { return m_cmdTopic; }

    private:
        char m_deviceId[32] = {};
        char m_uri[160] = {};
        char m_telemetryTopic[128] = {};
        char m_cmdTopic[128] = {};
        
        // 💡 AWS 내장 섀도우 토픽 버퍼 추가
        char m_shadowUpdateTopic[128] = {};
        char m_shadowDeltaTopic[128] = {};

        char m_rxTopic[128] = {};
        char m_rxPayload[2048] = {};
        int  m_rxTotalLen = 0;
        bool m_connected = false;
        CmdCallback m_cmdCb;

        std::string m_rootCaPemStr;
        std::string m_certPemStr;
        std::string m_keyPemStr;

        esp_mqtt_client_handle_t m_client = nullptr;

        uint32_t m_lastSampleTime = 0;       
        float m_temperatureItems[10] = {0.0f};    
        float m_humidityItems[10] = {0.0f};   
        uint32_t m_timestampItems[10] = {0};    // 💡 개별 데이터 샘플링 시간을 저장할 배열 추가
        int m_ItemCount = 0;               

        bool readFileToString(const char* path, std::string& outStr);
        void handleMqttEvent(int32_t eventId, void* eventData);
        static void mqttEventHandler(void* handlerArgs, esp_event_base_t base, int32_t eventId, void* eventData);
    };
}