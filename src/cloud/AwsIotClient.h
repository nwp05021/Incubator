#pragma once
#include <functional>
#include <cstdint>
#include <string>

#ifdef INCUBATOR_ENABLE_CLOUD
#include <mqtt_client.h>
#endif

// PlanStorage 전방 선언을 통해 의존성 주입 구조 형성
namespace incubator::storage {
    class PlanStorage;
}

namespace incubator::cloud
{
    using CmdCallback = std::function<void(const char* topic, const char* payload)>;

    class AwsIotClient
    {
    public:
        static constexpr int      kMqttPort = 8883;
        static constexpr int      kMqttQos = 1;

        /**
         * @brief AWS IoT Client 초기화 및 PlanStorage 연동 인증서 로드
         * @param endpoint AWS IoT Core 데이터 엔드포인트 주소
         * @param deviceId 사물 이름 (Thing Name)
         * @param storage 파일 시스템 마운트 상태를 확인할 PlanStorage 객체 참조
         * @return 초기화 및 인증서 로드 성공 여부
         */
        bool init(const char* endpoint, const char* deviceId, storage::PlanStorage& storage);

        void tick(uint32_t now, float currentTemp, float currentHumidity);
        bool publish(const char* topic, const char* json);
        bool publishTelemetry(const char* json);
        bool publishHealth(const char* json, bool retain = false);
        bool isConnected() const { return m_connected; };
        void setCmdCallback(CmdCallback cb) { m_cmdCb = cb; }
        const char* telemetryTopic() const { return m_telemetryTopic; }
        const char* healthTopic() const { return m_healthTopic; }
        const char* commandTopic() const { return m_cmdTopic; }

    private:
        char m_deviceId[32] = {};
        char m_uri[160] = {};
        char m_cmdTopic[128] = {};
        char m_telemetryTopic[128] = {};
        char m_healthTopic[128] = {};
        char m_rxTopic[128] = {};
        char m_rxPayload[2048] = {};
        int  m_rxTotalLen = 0;
        bool m_connected = false;
        CmdCallback m_cmdCb;

        // esp_mqtt_client_init 동안 메모리가 유지되어야 하므로 멤버 변수로 보관
        std::string m_rootCaPemStr;
        std::string m_certPemStr;
        std::string m_keyPemStr;

        // --- 50초 배치 전송을 위한 변수 추가 ---
        uint32_t m_lastSampleTime = 0;       // 마지막 샘플링 타임 스탬프 (ms)
        float m_tempSamples[10] = {0.0f};    // 온도 데이터 10개 저장 배열
        float m_humidSamples[10] = {0.0f};   // 습도 데이터 10개 저장 배열
        int m_sampleCount = 0;               // 현재 수집된 데이터 개수

        // LittleFS 파일 읽기용 내부 헬퍼 함수
        bool readFileToString(const char* filepath, std::string& output);

#ifdef INCUBATOR_ENABLE_CLOUD
        esp_mqtt_client_handle_t m_client = nullptr;
        static void mqttEventHandler(void* handlerArgs,
                                     esp_event_base_t base,
                                     int32_t eventId,
                                     void* eventData);
        void handleMqttEvent(int32_t eventId, void* eventData);
#endif
    };
}