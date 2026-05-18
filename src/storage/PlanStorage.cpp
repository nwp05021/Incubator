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
    
    // [보정] 일수가 길어지거나 데이터가 늘어날 것에 대비해 버퍼 크기를 16KB로 넉넉하게 확장
    DynamicJsonDocument doc(16384);
    doc[kFTableVersion] = table.tableVersion;
    doc[kFLastUpdatedAt] = table.lastUpdatedAt;
    doc[kFRowCount] = table.rowCount;
    
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

    File file = LittleFS.open(kPlanPath, FILE_WRITE);
    if (!file) {
        ESP_LOGE(TAG, "쓰기를 위한 plan.json 오픈 실패");
        return false;
    }
    
    if (serializeJson(doc, file) == 0) {
        file.close();
        ESP_LOGE(TAG, "serializeJson 작성 실패");
        return false;
    }
    
    file.close();
    return true;
}

/**
 * @brief LittleFS의 JSON 파일을 읽어와 구조체 메모리에 로드
 */
bool PlanStorage::load(domain::IncubationPlanTable& table)
{
    if (!m_mounted) {
        ESP_LOGE(TAG, "LittleFS가 마운트되지 않은 상태에서 load 시도됨!");
        return false;
    }
    if (!exists()) {
        ESP_LOGW(TAG, "plan.json 파일이 존재하지 않습니다.");
        return false;
    }

    File file = LittleFS.open(kPlanPath, FILE_READ);
    if (!file) {
        ESP_LOGE(TAG, "읽기를 위한 plan.json 오픈 실패");
        return false;
    }

    // [보정] 로드 시 역직렬화 오버헤드를 견디기 위해 버퍼 크기를 16KB로 확장
    DynamicJsonDocument doc(16384);
    DeserializationError err = deserializeJson(doc, file);
    file.close(); 

    if (err) {
        // 이 로그가 시리얼 모니터에 찍힌다면 100% JSON 버퍼 용량 부족 에러입니다.
        ESP_LOGE(TAG, "deserializeJson 파싱 실패 원인: %s", err.c_str());
        return false;
    }

    if (!doc.containsKey(kFRows)) return false;
    
    table.clear();
    table.tableVersion = doc[kFTableVersion] | 0;
    table.lastUpdatedAt = doc[kFLastUpdatedAt] | 0U;
    table.rowCount = 0;

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

    // [보정] 데이터는 다 읽었으나 비즈니스 룰 검증에서 탈락했는지 판별하기 위한 추적 로그 추가
    bool valid = table.isValid();
    if (!valid) {
        ESP_LOGE(TAG, "JSON 파싱은 성공했으나, table.isValid() 도메인 검증에서 실패함! (rowCount: %u)", table.rowCount);
    }

    return valid;
}

} // namespace incubator::storage