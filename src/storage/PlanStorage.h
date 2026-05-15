#pragma once
#include "domain/IncubationPlanTable.h"

namespace incubator::storage
{
    class PlanStorage
    {
    public:
        // [변경] 기존 /spiffs 경로에서 LittleFS 표준 마운트 포인트로 교체
        static constexpr const char* kMountPoint = "/littlefs";
        static constexpr const char* kPlanPath   = "/littlefs/plan.json";

        /**
         * @brief LittleFS 파일 시스템을 초기화하고 마운트합니다.
         * @return 마운트 성공 시 true, 실패 시 false
         */
        bool init();

        /**
         * @brief 배양 계획 데이터를 JSON 문자열로 직렬화하여 LittleFS에 저장합니다.
         */
        bool save(const domain::IncubationPlanTable& table);

        /**
         * @brief LittleFS에서 JSON 파일을 읽어와 역직렬화(파싱) 후 구조체에 로드합니다.
         */
        bool load(domain::IncubationPlanTable& table);

        /**
         * @brief 파일 시스템 내에 배양 계획 파일(plan.json)이 존재하는지 조회합니다.
         */
        bool exists() const;

        /**
         * @brief 기존에 저장된 배양 계획 파일을 안전하게 삭제합니다.
         */
        bool erase();

        /**
         * @brief 파일 시스템의 초기화(마운트) 완료 여부를 확인합니다.
         */
        bool isInitialized() const { return m_mounted; }

    private:
        bool m_mounted = false; // 파일 시스템 마운트 상태 플래그
    };
}