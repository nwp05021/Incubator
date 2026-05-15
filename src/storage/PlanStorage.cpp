#include "storage/PlanStorage.h"
#include <LittleFS.h> // [변경] SPIFFS.h 대신 LittleFS.h 참조
#include <ArduinoJson.h>
#include <esp_log.h>
#include <cstring>

static const char* TAG = "PlanStorage";

// JSON 문서 역직렬화/직렬화 시 매핑될 Key 데이터 정의
static constexpr const char* kFTableVersion  = "tableVersion";
static constexpr const char* kFLastUpdatedAt = "lastUpdatedAt";
static constexpr const char* kFRowCount      = "rowCount";
static constexpr const char* kFRows          = "rows";
static constexpr const char* kFDay           = "day";
static constexpr const char* kFTemp          = "targetTempC";
static constexpr const char* kFHumi          = "targetHumidityPct";
static constexpr const char* kFTurning       = "turningEnabled";
static constexpr const char* kFInterval      = "turningIntervalMin";
static constexpr const char* kFVent           = "ventFanEnabled";
static constexpr const char* kFOverride      = "userOverridden";

namespace incubator::storage
{

/**
 * @brief LittleFS 초기화 및 파일 시스템 마운트 실행
 */
bool PlanStorage::init()
{
    // LittleFS.begin(formatOnFail, local_path)
    // 첫 번째 인자가 true이므로 마운트 실패(예: 미포맷 상태) 시 자동으로 포맷을 수행합니다.
    if (!LittleFS.begin(true, kMountPoint)) {
        ESP_LOGE(TAG, "LittleFS 마운트 실패");
        m_mounted = false;
        return false;
    }
    m_mounted = true;
    
    // 마운트 성공 후 파일 시스템의 전체 용량 및 현재 사용량을 로그로 출력
    ESP_LOGI(TAG, "LittleFS 마운트 완료, total=%u used=%u bytes",
             static_cast<unsigned>(LittleFS.totalBytes()),
             static_cast<unsigned>(LittleFS.usedBytes()));
    return true;
}

bool PlanStorage::exists() const
{
    if (!m_mounted) return false;
    return LittleFS.exists(kPlanPath); // 파일 존재 여부 헬퍼
}

bool PlanStorage::erase()
{
    if (!m_mounted) return false;
    if (!exists()) return true;       // 파일이 없으면 삭제 행위가 성공한 것과 같음
    return LittleFS.remove(kPlanPath); // 파일 물리적 삭제
}

/**
 * @brief 구조체 객체를 파싱하여 JSON 형태로 LittleFS에 쓰기
 */
bool PlanStorage::save(const domain::IncubationPlanTable& table)
{
    if (!m_mounted) return false;
    
    // 고정 메모리 풀을 사용하는 DynamicJsonDocument 할당
    DynamicJsonDocument doc(8192);
    doc[kFTableVersion] = table.tableVersion;
    doc[kFLastUpdatedAt] = table.lastUpdatedAt;
    doc[kFRowCount] = table.rowCount;
    
    // 가변 배열 데이터를 JSON Array 구조로 레이아웃 생성
    auto rows = doc.createNestedArray(kFRows);
    for (uint16_t i = 0; i < table.rowCount; ++i) {
        auto row = rows.createNestedObject();
        row[kFDay] = table.rows[i].day;
        row[kFTemp] = table.rows[i].targetTempC;
        row[kFHumi] = table.rows[i].targetHumidityPct;
        row[kFTurning] = table.rows[i].turningEnabled;
        row[kFInterval] = table.rows[i].turningIntervalMin;
        row[kFVent] = table.rows[i].ventFanEnabled;
        row[kFOverride] = table.rows[i].userOverridden;
    }

    // FILE_WRITE 모드로 파일을 오픈하여 스트림 획득 (기존 데이터는 덮어씌워짐)
    File file = LittleFS.open(kPlanPath, FILE_WRITE);
    if (!file) {
        ESP_LOGE(TAG, "쓰기를 위한 plan.json 오픈 실패");
        return false;
    }
    
    // JSON Object 데이터를 파일 스트림에 직렬화하여 기록
    if (serializeJson(doc, file) == 0) {
        file.close(); // 에러 발생 시 파일 핸들을 반드시 닫아줌
        ESP_LOGE(TAG, "serializeJson 작성 실패");
        return false;
    }
    
    file.close(); // 정상 작성 완료 후 닫기
    return true;
}

/**
 * @brief LittleFS의 JSON 파일을 읽어와 구조체 메모리에 로드
 */
bool PlanStorage::load(domain::IncubationPlanTable& table)
{
    if (!m_mounted) return false;
    if (!exists()) return false;

    // FILE_READ 모드로 파일 오픈
    File file = LittleFS.open(kPlanPath, FILE_READ);
    if (!file) {
        ESP_LOGE(TAG, "읽기를 위한 plan.json 오픈 실패");
        return false;
    }

    // 아두이노 JSON 파싱 엔진 가동
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, file);
    file.close(); // 데이터 파싱 완료 직후 파일 스트림은 즉시 해제하는 것이 안전함

    if (err) {
        ESP_LOGE(TAG, "deserializeJson 파싱 실패: %s", err.c_str());
        return false;
    }

    // 데이터 무결성 검증 (배열 키가 누락되었는지 확인)
    if (!doc.containsKey(kFRows)) return false;
    
    // 타겟 구조체 초기화 후 기본 필드 대입
    table.clear();
    table.tableVersion = doc[kFTableVersion] | 0;
    table.lastUpdatedAt = doc[kFLastUpdatedAt] | 0U;
    table.rowCount = 0;

    // JSON 내부 배열을 순회하며 구조체 버퍼 영역으로 복사 연산 수행
    auto rows = doc[kFRows].as<JsonArray>();
    for (JsonObject row : rows) {
        if (table.rowCount >= domain::IncubationPlanTable::kMaxRows) break;
        
        auto& target = table.rows[table.rowCount];
        target.day = row[kFDay] | 0;
        target.targetTempC = row[kFTemp] | 0.0f;
        target.targetHumidityPct = row[kFHumi] | 0.0f;
        target.turningEnabled = row[kFTurning] | false;
        target.turningIntervalMin = row[kFInterval] | 0;
        target.ventFanEnabled = row[kFVent] | false;
        target.userOverridden = row[kFOverride] | false;
        
        table.rowCount++;
    }

    // 최종 로드된 도메인 데이터 구조체의 비즈니스 룰 검증 결과 반환
    return table.isValid();
}

} // namespace incubator::storage