#pragma once
#include <cstdint>
#include <cstring> // [추장] strncpy 사용을 위해 필수로 추가

namespace incubator::ui
{
    enum class EditField : uint8_t { None, Day, Temp, Humidity, Turning };
    enum class UiScreen : uint8_t
    {
        Main,
        Menu,
        StartDate,
        Preset,
        PlanList,
        PlanEdit,
        Manual,
        AwsTest,
        WifiReset,
        RebootConfirm,
        FactoryReset,
        System,
        BleProvisioning // 🔥 BLE 동작을 매핑하기 위해 추가합니다.
    };

    // 🔥 하나의 메뉴 정보를 담는 구조체입니다.
    struct MenuItem {
        const char* name;
        const char* desc;
        UiScreen targetScreen;
    };

    // 🔥 앞으로 메뉴 추가/삭제/순서 변경은 오직 이 배열에서만 하시면 됩니다!
    static constexpr MenuItem kMainMenuItems[] = {
        { "부화 시작일", "날짜 관리", UiScreen::StartDate },
        { "프리셋 선택", "종 선택", UiScreen::Preset },
        { "일별 설정", "스케줄 표", UiScreen::PlanList },
        { "수동 테스트", "배선 점검", UiScreen::Manual },
        { "Aws 테스트", "AWS 연결", UiScreen::AwsTest },
        { "WiFi 리셋", "인증 삭제", UiScreen::WifiReset },
        { "BLE 설정", "QR 연결", UiScreen::BleProvisioning },
        { "재부팅", "시스템", UiScreen::RebootConfirm },
        { "공장 초기화", "10초 유지", UiScreen::FactoryReset }
    };
    static constexpr uint8_t kMainMenuCount = sizeof(kMainMenuItems) / sizeof(kMainMenuItems[0]);

    // UiModel.h 파일 내 관련 배열 수정/확장
    struct AwsPublishEndpoint {
        const char* name;
        const char* topicFormat;
    };

    static constexpr AwsPublishEndpoint kAwsPublishEndpoints[] = {
        { "Telemetry", "incubator/v1/%s/telemetry" },
        { "Health",    "incubator/v1/%s/health" },
        { "Shadow",    "$aws/things/%s/shadow/update" },
        { "Cmd",       "incubator/v1/%s/cmd" }, // ◀ 내부 코어와 싱크를 맞추기 위해 명세에 추가
        { "Test",      "incubator/v1/%s/test" }
    };

    static constexpr uint8_t kAwsPublishEndpointCount =
        sizeof(kAwsPublishEndpoints) / sizeof(kAwsPublishEndpoints[0]);

    // 💡 [단일 진실 원칙 준수] 엔드포인트 정보 검색 책임을 UiModel이 일괄적으로 가집니다.
    inline int findAwsEndpointIndex(const char* name) {
        for (uint8_t i = 0; i < kAwsPublishEndpointCount; ++i) {
            if (std::strcmp(kAwsPublishEndpoints[i].name, name) == 0) {
                return i; 
            }
        }
        return 0; // 찾지 못했을 경우 기본값(Telemetry)의 인덱스 반환
    }

    inline const char* findAwsTopicFormat(const char* name) {
        for (uint8_t i = 0; i < kAwsPublishEndpointCount; ++i) {
            if (std::strcmp(kAwsPublishEndpoints[i].name, name) == 0) {
                return kAwsPublishEndpoints[i].topicFormat;
            }
        }
        return nullptr; // 찾지 못했을 경우
    }

    struct PlanListItem
    {
        uint16_t day = 0;
        float    targetTempC = 0.0f;
        float    targetHumidPct = 0.0f;
        bool     turningEnabled = false;
        uint16_t intervalMin = 0;
        bool     overridden = false;
    };

    struct UiModel
    {
        float    displayTempC      = 0.0f;
        float    displayHumidPct   = 0.0f;
        float    targetTempC       = 37.5f;
        float    targetHumidPct    = 55.0f;
        bool     heaterOn          = false;
        bool     humidifierOn      = false;
        bool     turnerOn          = false;
        bool     fanOn             = false;
        bool     tempAlarm         = false;
        bool     humiAlarm         = false;
        bool     tempSensorFault   = false;
        bool     humiSensorFault   = false;
        bool     tempSensorWarning = false;
        bool     humiSensorWarning = false;
        uint16_t currentDay        = 0;
        uint16_t totalDays         = 21;
        uint8_t  progressPct       = 0;
        bool     batchActive       = false;
        bool     lockdownActive    = false;
        bool     turningEnabled    = true;
        uint16_t nextTurningInMin  = 0;
        uint16_t lockdownStartDay  = 19;
        uint32_t batchStartEpoch   = 0;

        uint16_t editDay           = 1;
        float    editTempC         = 37.5f;
        float    editHumidPct      = 55.0f;
        bool     editTurning       = true;
        uint16_t editIntervalMin   = 120;
        bool     editOverridden    = false;
        EditField activeEditField  = EditField::None;

        uint32_t bootCount         = 0;
        uint32_t uptimeMs          = 0;
        bool     cloudConnected    = false;
        bool     manualMode        = false;
        char     ipAddress[16]     = {};
        char     batchId[16]       = {};
        char     fwVersion[12]     = "v1.0.0";

        UiScreen screen            = UiScreen::Main;
        uint8_t  activePage        = 0;
        bool     menuOpen          = false;
        uint8_t  menuCursor        = 0;
        bool     safeMode          = false;
        int8_t   manualCursor      = 0;
        uint8_t  confirmCursor     = 0;
        uint8_t  presetCursor      = 0;
        bool     presetConfirm     = false;
        bool     editMode          = false;
        uint8_t  fieldCursor       = 0;
        uint8_t  planListCount     = 0;
        PlanListItem planList[5]   = {};
        uint16_t editBatchYear     = 2026;
        uint8_t  editBatchMonth    = 1;
        uint8_t  editBatchDay      = 1;
        uint8_t  factoryProgressPct = 0;
        bool     factoryReady      = false;
        char     actionMessage[40] = {};

        bool     wifiConfigured    = false;
        bool     wifiConnected     = false; // 🔥 [추가] 실제 WiFi 연결 여부 플래그
        bool     localMode         = false;
        bool     provisioningActive = false;
        bool     provisioningSucceeded = false;
        bool     provisioningFailed = false;
        uint32_t provisioningRemainingMs = 0;
        char     provisioningName[32] = {};
        char     provisioningPop[16] = {};
        char     provisioningMessage[40] = {};

        // [이동] 컴파일 에러 해결을 위해 구조체 내부 멤버 변수로 격상 배치
        bool awsTestRunning = false;
        uint8_t awsEndpointCursor = 0;
        uint32_t awsTestCount = 0;
        bool awsLastResult = false;
        char awsTestLog[4][64] = {"", "", "", ""};

        // [이동] 구조체 내부 멤버 함수로 배치하여 멤버 변수에 정상 접근하도록 수정
        void pushAwsLog(const char* msg) {
            for (int i = 0; i < 3; ++i) {
                std::strncpy(awsTestLog[i], awsTestLog[i + 1], sizeof(awsTestLog[i]));
            }
            std::strncpy(awsTestLog[3], msg, sizeof(awsTestLog[3]) - 1);
            awsTestLog[3][sizeof(awsTestLog[3]) - 1] = '\0'; // 안전한 널 종료 보장
        }    
    };
}
