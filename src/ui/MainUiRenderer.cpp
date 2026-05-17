#include "ui/MainUiRenderer.h"
#include "ui/UiLayout.h"
#include "ui/UiColors.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace incubator::ui
{
namespace
{
// --- [리팩토링] 하드코딩 원색을 제거하고 럭셔리 세만틱 컬러로 조율 ---
    static constexpr uint32_t kPanel      = 0x2146U; // 카드가 배경 위로 플로팅된 느낌을 주는 차콜 그레이 솔리드 박스
    static constexpr uint32_t kPanelSoft  = 0x31CDU; // 패널 테두리를 부드럽게 감싸는 미디엄 슬레이트 틴트
    static constexpr uint32_t kOk         = 0x3666U; // 에메랄드 민트 그린
    static constexpr uint32_t kWarn       = 0xECE4U; // 소프트 파스텔 골드
    static constexpr uint32_t kDanger     = 0xD906U; // 크림슨 로즈 레드
    static constexpr uint32_t kTeal       = 0x2CDFU; // 모던하고 스마트한 감성의 세련된 사이언/인포 블루
    static constexpr uint8_t  kMenuCount  = 8;

    void hashBytes(uint32_t& hash, const void* data, size_t len)
    {
        const auto* p = static_cast<const uint8_t*>(data);
        while (len-- > 0U) {
            hash ^= *p++;
            hash *= 16777619U;
        }
    }

    template <typename T>
    void hashValue(uint32_t& hash, const T& value)
    {
        hashBytes(hash, &value, sizeof(T));
    }

    void hashString(uint32_t& hash, const char* text)
    {
        if (!text) {
            uint8_t zero = 0;
            hashValue(hash, zero);
            return;
        }
        while (*text) {
            uint8_t c = static_cast<uint8_t>(*text++);
            hashValue(hash, c);
        }
        uint8_t zero = 0;
        hashValue(hash, zero);
    }

    int16_t q10(float value)
    {
        return static_cast<int16_t>(std::lroundf(value * 10.0f));
    }

    int16_t q1(float value)
    {
        return static_cast<int16_t>(std::lroundf(value));
    }

    const char* screenTitle(const UiModel& model)
    {
        if (model.screen == UiScreen::Menu) return "메뉴";
        if (model.screen == UiScreen::StartDate) return "부화 시작일";
        if (model.screen == UiScreen::Preset) return "프리셋 선택";
        if (model.screen == UiScreen::PlanList || model.screen == UiScreen::PlanEdit) return "부화 스케줄";
        if (model.screen == UiScreen::Manual) return "수동 테스트";
        if (model.screen == UiScreen::System) return "SYSTEM";
        if (model.screen == UiScreen::WifiReset) return "WiFi 정보 리셋";
        if (model.screen == UiScreen::RebootConfirm) return "시스템 재부팅";
        if (model.screen == UiScreen::FactoryReset) return "공장 초기화";
        switch (model.activePage) {
            case 1: return "상태 정보";
            case 2: return "도움말";
            default: return "부화기";
        }
    }

    bool hasUtf8(const char* text)
    {
        if (!text) return false;
        while (*text) {
            if ((static_cast<unsigned char>(*text) & 0x80U) != 0U) return true;
            ++text;
        }
        return false;
    }

    void formatDateTime(uint32_t uptimeMs, char* out, size_t len)
    {
        std::time_t now = std::time(nullptr);
        std::tm tmv = {};
        bool valid = false;
#if defined(_WIN32)
        valid = localtime_s(&tmv, &now) == 0;
#else
        if (std::tm* local = std::localtime(&now)) {
            tmv = *local;
            valid = true;
        }
#endif
        if (valid && tmv.tm_year >= 120) {
            std::snprintf(out, len, "%02d-%02d %02d:%02d:%02d",
                          tmv.tm_mon + 1, tmv.tm_mday,
                          tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
            return;
        }

        uint32_t seconds = uptimeMs / 1000U;
        uint32_t minutes = seconds / 60U;
        uint32_t hours = (minutes / 60U) % 24U;
        std::snprintf(out, len, "-- -- %02u:%02u:%02u",
                  (unsigned int)hours, 
                  (unsigned int)(minutes % 60U), 
                  (unsigned int)(seconds % 60U));    
    }

    void formatDate(char* out, size_t len)
    {
        std::time_t now = std::time(nullptr);
        std::tm tmv = {};
        bool valid = false;
#if defined(_WIN32)
        valid = localtime_s(&tmv, &now) == 0;
#else
        if (std::tm* local = std::localtime(&now)) {
            tmv = *local;
            valid = true;
        }
#endif
        if (valid && tmv.tm_year >= 120) {
            std::snprintf(out, len, "%04d-%02d-%02d",
                          tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday);
        } else {
            std::snprintf(out, len, "---- -- --");
        }
    }

    const char* presetName(uint8_t i)
    {
        switch (i) {
            case 0: return "닭";
            case 1: return "오리";
            case 2: return "메추리";
            case 3: return "거위";
            default: return "닭";
        }
    }

    // --- 1. 상단 바 전용 해시 검사 ---
    uint32_t getHeaderHash(const UiModel& model, uint32_t nowMs)
    {
        uint32_t hash = 2166136261U;
        hashValue(hash, model.screen);
        hashValue(hash, model.activePage);
        hashValue(hash, nowMs / 1000U);
        hashValue(hash, model.uptimeMs / 1000U);
        hashValue(hash, model.cloudConnected);
        hashValue(hash, model.wifiConfigured);
        hashValue(hash, model.tempAlarm);
        hashValue(hash, model.humiAlarm);
        hashValue(hash, model.tempSensorFault);
        hashValue(hash, model.humiSensorFault);
        hashValue(hash, model.tempSensorWarning);
        hashValue(hash, model.humiSensorWarning);
        return hash;
    }

    // --- 2. 하단 바 전용 해시 검사 ---
    uint32_t getFooterHash(const UiModel& model, uint32_t nowMs)
    {
        uint32_t hash = 2166136261U;
        hashValue(hash, model.screen);
        hashValue(hash, model.currentDay);
        hashValue(hash, model.totalDays);
        hashValue(hash, model.heaterOn);
        hashValue(hash, model.humidifierOn);
        hashValue(hash, model.turnerOn);
        hashValue(hash, model.fanOn);
        if (model.heaterOn || model.humidifierOn || model.turnerOn || model.fanOn) {
            hashValue(hash, nowMs / 500U);
        }
        return hash;
    }

    // --- 3. 중앙 메인 컨텐츠 전용 해시 검사 ---
    uint32_t getPageHash(const UiModel& model, uint32_t nowMs)
    {
        uint32_t hash = 2166136261U;
        hashValue(hash, model.screen);
        hashValue(hash, model.activePage);
        hashValue(hash, model.lockdownActive);
        hashValue(hash, model.safeMode);

        hashValue(hash, q10(model.displayTempC));
        hashValue(hash, q1(model.displayHumidPct));
        hashValue(hash, q10(model.targetTempC));
        hashValue(hash, q1(model.targetHumidPct));
        hashValue(hash, model.turningEnabled);
        hashValue(hash, model.nextTurningInMin);

        hashValue(hash, model.menuCursor);
        hashValue(hash, model.manualCursor);
        hashValue(hash, model.confirmCursor);
        hashValue(hash, model.presetCursor);
        hashValue(hash, model.presetConfirm);
        hashValue(hash, model.editMode);
        hashValue(hash, model.fieldCursor);
        hashValue(hash, model.editBatchYear);
        hashValue(hash, model.editBatchMonth);
        hashValue(hash, model.editBatchDay);
        hashValue(hash, model.editDay);
        hashValue(hash, q10(model.editTempC));
        hashValue(hash, q1(model.editHumidPct));
        hashValue(hash, model.editTurning);
        hashValue(hash, model.editIntervalMin);
        hashValue(hash, model.activeEditField);
        hashValue(hash, model.planListCount);
        for (uint8_t i = 0; i < model.planListCount; ++i) {
            hashValue(hash, model.planList[i].day);
            hashValue(hash, q10(model.planList[i].targetTempC));
            hashValue(hash, q1(model.planList[i].targetHumidPct));
            hashValue(hash, model.planList[i].turningEnabled);
            hashValue(hash, model.planList[i].intervalMin);
            hashValue(hash, model.planList[i].overridden);
        }
        hashValue(hash, model.factoryProgressPct);
        hashValue(hash, model.factoryReady);
        hashString(hash, model.actionMessage);

        hashValue(hash, model.provisioningActive);
        hashValue(hash, model.provisioningSucceeded);
        hashValue(hash, model.provisioningFailed);
        hashValue(hash, model.provisioningRemainingMs / 1000U);
        hashString(hash, model.provisioningName);
        hashString(hash, model.provisioningPop);
        hashString(hash, model.provisioningMessage);

        if (model.screen == UiScreen::System) {
            hashValue(hash, model.bootCount);
            hashValue(hash, model.batchActive);
            hashString(hash, model.ipAddress);
        }
        return hash;
    }
} // namespace

// --- 여기서부터 MainUiRenderer의 멤버 함수 구현 시작 ---

void MainUiRenderer::render(uint32_t nowMs)
{
    if (m_hasRendered && nowMs - m_lastRenderMs < 30U) return;

    m_renderNowMs = nowMs;
    m_lastRenderMs = nowMs;

    m_display.beginFrame();

    if (m_model.provisioningActive) {
        if (!m_wasProvisioning) {
            m_provisioningRenderer.reset();
            m_wasProvisioning = true;
        }
        m_provisioningRenderer.render(nowMs);
        m_hasRendered = true;
        m_display.endFrame();
        return;
    }

    if (m_wasProvisioning) {
        m_wasProvisioning = false;
        m_firstRender = true;
    }

    const uint32_t currentHeaderHash = getHeaderHash(m_model, nowMs);
    const uint32_t currentPageHash   = getPageHash(m_model, nowMs);
    const uint32_t currentFooterHash = getFooterHash(m_model, nowMs);

    bool isHeaderDirty = m_firstRender || (currentHeaderHash != m_lastHeaderHash);
    bool isPageDirty   = m_firstRender || (currentPageHash != m_lastPageHash);
    bool isFooterDirty = m_firstRender || (currentFooterHash != m_lastFooterHash);

    m_lastHeaderHash = currentHeaderHash;
    m_lastPageHash   = currentPageHash;
    m_lastFooterHash = currentFooterHash;
    m_firstRender    = false;
    m_hasRendered    = true;

    if (isHeaderDirty) {
        drawStatusBar();
    }

    if (isPageDirty) {
        m_display.fillRect(0, 26, Layout::kScreenW, 189, Color::kBg);

        switch (m_model.screen) {
            case UiScreen::Main:
                if (m_model.activePage == 0) renderPage0();
                else if (m_model.activePage == 1) renderPage1();
                else renderHelp();
                break;
            case UiScreen::Menu: renderMenu(); break;
            case UiScreen::StartDate: renderStartDate(); break;
            case UiScreen::Preset: renderPreset(); break;
            case UiScreen::PlanList: renderPlanList(); break;
            case UiScreen::PlanEdit: renderPlanEdit(); break;
            case UiScreen::Manual: renderManual(); break;
            case UiScreen::WifiReset:
                renderConfirm("WiFi 정보 리셋", "저장된 WiFi 인증정보를 삭제합니다.", "삭제 후 BLE 설정이 필요합니다.");
                break;
            case UiScreen::RebootConfirm:
                renderConfirm("시스템 재부팅", "장치를 다시 시작합니다.", "진행 중 제어가 잠시 중단됩니다.");
                break;
            case UiScreen::FactoryReset: renderFactoryReset(); break;
            case UiScreen::System: renderPage4(); break;
        }

        if (m_model.safeMode) {
            m_display.fillRect(72, 102, 176, 34, kDanger);
            m_display.setTextSize(2);
            m_display.setTextColor(Color::kText, kDanger);
            m_display.drawText(104, 112, "SAFE MODE");
        }
    }

    if (isFooterDirty) {
        renderFooter();
    }

    m_display.endFrame();
}

void MainUiRenderer::drawStatusBar()
{
    char buffer[32];
    m_display.fillRect(0, 0, Layout::kScreenW, 26, Color::kHeader);
    m_display.setTextColor(Color::kText, Color::kHeader);

    if (m_model.screen == UiScreen::Main && m_model.activePage == 0U) {
        m_display.setTextSize(2);
        formatDateTime(m_model.uptimeMs, buffer, sizeof(buffer));
        m_display.drawText(8, 5, buffer);
    } else {
        m_display.setTextSize(1);
        m_display.drawText(10, 6, screenTitle(m_model));
        formatDate(buffer, sizeof(buffer));
        m_display.setTextColor(Color::kTextDim, Color::kHeader);
        m_display.drawText(236, 6, buffer);
    }

    if (m_model.screen == UiScreen::Main && m_model.activePage == 0U) {
        drawSignalBars(258, 5, m_model.cloudConnected, m_model.wifiConfigured);
        const bool hardAlarm = m_model.tempAlarm || m_model.humiAlarm ||
                               m_model.tempSensorFault || m_model.humiSensorFault;
        const bool sensorWarning = m_model.tempSensorWarning || m_model.humiSensorWarning;
        const uint32_t warnColor = hardAlarm ? kDanger : (sensorWarning ? kWarn : Color::kOffIcon);
        m_display.drawRect(292, 6, 18, 14, warnColor);
        m_display.setTextSize(1);
        m_display.setTextColor(warnColor, Color::kHeader);
        m_display.drawText(298, 7, "!");
    }
    m_display.setTextSize(1);
    m_display.drawLine(0, 25, Layout::kScreenW, 25, Color::kDivider);
}

void MainUiRenderer::renderHeader()
{
    m_display.fillRect(0, 24, Layout::kScreenW, 26, Color::kHeader);
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kText, Color::kHeader);
    m_display.drawText(12, 29, screenTitle(m_model));
    m_display.setTextColor(Color::kTextDim, Color::kHeader);
    m_display.drawText(244, 33, "2026-05-05");
    m_display.drawLine(0, 49, Layout::kScreenW, 49, Color::kDivider);
}

void MainUiRenderer::renderFooter()
{
    m_display.drawLine(0, 215, Layout::kScreenW, 215, Color::kDivider);
    m_display.fillRect(0, 216, Layout::kScreenW, 24, Color::kFooter);
    
    if (m_model.screen == UiScreen::Main) {
        char progress[20];
        drawStatusIcons(); // 개선된 구동 아이콘 드로잉
        std::snprintf(progress, sizeof(progress), "%u일차/%u", m_model.currentDay, m_model.totalDays);
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kText, Color::kFooter);
        m_display.drawText(244, 223, progress); // Y축 배치 살짝 상향 조정 (가독성 벨런스)
    } else {
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kFooter);
        const char* hint = "회전: 이동  클릭: 선택  길게: 뒤로";
        if (m_model.screen == UiScreen::Manual) hint = "회전: 선택  클릭: 토글  길게: 종료";
        if (m_model.screen == UiScreen::PlanEdit) hint = "회전: 이동  클릭: 편집/확정";
        if (m_model.screen == UiScreen::FactoryReset) hint = "버튼 10초 유지: 실행  길게: 뒤로";
        m_display.drawText(12, 223, hint);
    }
}

// MainUiRenderer.cpp 내부의 void MainUiRenderer::renderPage0() 함수 전체 교체

void MainUiRenderer::renderPage0()
{
    char buffer[48];

    // 1. 중앙 분리선 (은은하고 얇은 그리드 라인으로 세련미 유지)
    m_display.drawLine(160, 36, 160, 154, Color::kDivider);
    
    // 2. 텍스트 하단을 정갈하게 받쳐주는 메탈릭 가이드 앵커선
    m_display.drawLine(20, 146, 140, 146, kPanelSoft);
    m_display.drawLine(180, 146, 300, 146, kPanelSoft);

    // ==================== [LEFT] 온도 대시보드 영역 (0 ~ 160px) ====================
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(22, 42, "TEMPERATURE"); 

    if (m_model.tempSensorFault) {
        m_display.setTextSize(1);
        m_display.setTextColor(kDanger, Color::kBg);
        m_display.drawText(54, 74, "---");
        m_display.drawText(56, 122, "FAULT");
    } else {
        // [핵심 변경] drawNumberText 고유의 대형 폰트가 가장 아름답게 표현되는 배율 1 설정
        m_display.setTextSize(1); 
        m_display.setTextColor(Color::kAccentTemp, Color::kBg);
        std::snprintf(buffer, sizeof(buffer), "%.1f", m_model.displayTempC);
        
        // 대형 숫자 폰트가 중앙선을 침범하지 않고 완벽히 안착하는 X축 황금 좌표
        m_display.drawNumberText(24, 74, buffer); 

        // 단위 'C' 기호를 수치 우측 여백에 정갈하게 정렬
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(126, 76, "C");
    }

    // ==================== [RIGHT] 습도 대시보드 영역 (160 ~ 320px) ====================
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(182, 42, "HUMIDITY");

    if (m_model.humiSensorFault) {
        m_display.setTextSize(1);
        m_display.setTextColor(kDanger, Color::kBg);
        m_display.drawText(214, 74, "---");
        m_display.drawText(216, 122, "FAULT");
    } else {
        // 습도 영역도 밸런스를 맞추기 위해 정갈한 기본 대형 폰트 배율 1 적용
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kAccentHumi, Color::kBg);
        std::snprintf(buffer, sizeof(buffer), "%.0f", m_model.displayHumidPct);
        
        // 습도 수치가 우측 그리드(320px) 내에 황금 비율로 배치되는 X축 좌표
        m_display.drawNumberText(198, 74, buffer);

        // 단위 '%' 기호 정렬
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(274, 76, "%");
    }

    // ==================== [BOTTOM] 하단 설정치(SET) 알약 모양 안내 라벨 ====================
    m_display.setTextSize(1);
    std::snprintf(buffer, sizeof(buffer), "SET %.1f C", m_model.targetTempC);
    drawPill(20, 172, 120, buffer, Color::kAccentTemp); // 가로 폭 정밀 피팅

    std::snprintf(buffer, sizeof(buffer), "SET %.0f %%", m_model.targetHumidPct);
    drawPill(180, 172, 120, buffer, Color::kAccentHumi);
}

void MainUiRenderer::renderPage1()
{
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%.1f C", m_model.targetTempC);
    drawRow(62, "목표 온도", buffer, false, Color::kAccentTemp);
    std::snprintf(buffer, sizeof(buffer), "%.0f %%", m_model.targetHumidPct);
    drawRow(94, "목표 습도", buffer, false, Color::kAccentHumi);
    drawRow(126, "전란", m_model.lockdownActive ? "LOCK" : (m_model.turningEnabled ? "ON" : "OFF"), false,
            m_model.lockdownActive ? kWarn : (m_model.turningEnabled ? kOk : Color::kOffIcon));
    std::snprintf(buffer, sizeof(buffer), "%u min", m_model.nextTurningInMin);
    drawRow(158, "다음 전란", buffer, false, kTeal);
}

void MainUiRenderer::renderHelp()
{
    drawRow(62, "기본", "회전으로 화면 전환", false, kTeal);
    drawRow(96, "메뉴", "길게 누르기", false, Color::kTextDim);
    drawRow(130, "수동", "테스트 후 자동 복귀", false, kWarn);
    drawRow(164, "WiFi", "BLE QR 설정 지원", false, Color::kAccentHumi);
}

void MainUiRenderer::renderPage2() { renderManual(); }
void MainUiRenderer::renderPage3() { renderPlanEdit(); }

void MainUiRenderer::renderPage4()
{
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%u s", (unsigned int)(m_model.uptimeMs / 1000U));
    drawRow(58, "Uptime", buffer, false, kTeal);
    std::snprintf(buffer, sizeof(buffer), "%u", (unsigned int)m_model.bootCount);
    drawRow(84, "Boot cnt", buffer, false, Color::kTextDim);
    drawRow(110, "Cloud", m_model.cloudConnected ? "ON" : "OFF", false, m_model.cloudConnected ? kOk : Color::kOffIcon);
    drawRow(136, "IP", m_model.ipAddress[0] ? m_model.ipAddress : "IP 없음", false, Color::kTextDim);
    drawRow(162, "Batch", m_model.batchActive ? "ACTIVE" : "STOP", false, m_model.batchActive ? kOk : Color::kOffIcon);
    drawRow(188, "SafeMode", m_model.safeMode ? "YES" : "NO", false, m_model.safeMode ? kDanger : kOk);
}

void MainUiRenderer::renderMenu()
{
    static constexpr const char* kNames[kMenuCount] = {
        "부화 시작일", "프리셋 선택", "일별 설정", "수동 테스트",
        "WiFi 리셋", "BLE 설정", "재부팅", "공장 초기화"
    };
    static constexpr const char* kDesc[kMenuCount] = {
        "날짜", "종 선택", "테이블", "배선 점검",
        "인증 삭제", "QR 연결", "시스템", "10초 유지"
    };

    for (uint8_t i = 0; i < kMenuCount; ++i) {
        int y = 54 + static_cast<int>(i) * 20;
        bool selected = (m_model.menuCursor == i);
        uint32_t bg = selected ? Color::kSelected : kPanel;
        m_display.fillRect(10, y, 300, 18, bg);
        m_display.drawRect(10, y, 300, 18, selected ? kTeal : kPanelSoft);
        m_display.fillRect(18, y + 5, 5, 8, selected ? kTeal : Color::kTextDim);
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kText, bg);
        m_display.drawText(30, y + 3, kNames[i]);
        m_display.setTextColor(Color::kTextDim, bg);
        m_display.drawText(190, y + 3, kDesc[i]);
    }
}

void MainUiRenderer::renderStartDate()
{
    char value[24];
    char dateText[24];
    std::snprintf(dateText, sizeof(dateText), "%04u-%02u-%02u",
                  m_model.editBatchYear, m_model.editBatchMonth, m_model.editBatchDay);

    m_display.setTextSize(1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(24, 38, "부화일 계산에 사용할 시작일을 등록합니다.");
    m_display.setTextColor(kTeal, Color::kBg);
    m_display.drawText(112, 58, dateText);

    struct Field {
        const char* label;
        const char* unit;
        uint32_t accent;
    };
    static constexpr Field kFields[] = {
        {"년도", "년", Color::kAccentTemp},
        {"월", "월", Color::kAccentHumi},
        {"일", "일", kTeal},
    };

    for (uint8_t i = 0; i < 3; ++i) {
        const int y = 82 + static_cast<int>(i) * 34;
        const bool selected = m_model.fieldCursor == i;
        const uint32_t bg = selected ? Color::kSelected : kPanel;
        const uint32_t border = selected ? (m_model.editMode ? kWarn : kFields[i].accent) : kPanelSoft;

        if (i == 0U) std::snprintf(value, sizeof(value), "%04u", m_model.editBatchYear);
        else if (i == 1U) std::snprintf(value, sizeof(value), "%02u", m_model.editBatchMonth);
        else std::snprintf(value, sizeof(value), "%02u", m_model.editBatchDay);

        m_display.fillRect(28, y, 264, 28, bg);
        m_display.drawRect(28, y, 264, 28, border);
        m_display.fillRect(36, y + 7, 6, 14, kFields[i].accent);
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, bg);
        m_display.drawText(54, y + 6, kFields[i].label);
        m_display.setTextSize(2);
        m_display.setTextColor(Color::kText, bg);
        m_display.drawText(146, y + 5, value);
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, bg);
        m_display.drawText(226, y + 9, kFields[i].unit);
    }

    const uint32_t saveBg = (m_model.fieldCursor == 3U) ? Color::kSelected : kPanel;
    const uint32_t cancelBg = (m_model.fieldCursor == 4U) ? Color::kSelected : kPanel;
    m_display.fillRect(70, 190, 78, 20, saveBg);
    m_display.drawRect(70, 190, 78, 20, (m_model.fieldCursor == 3U) ? kOk : kPanelSoft);
    m_display.fillRect(172, 190, 78, 20, cancelBg);
    m_display.drawRect(172, 190, 78, 20, (m_model.fieldCursor == 4U) ? Color::kOffIcon : kPanelSoft);
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kText, saveBg);
    m_display.drawText(94, 194, "등록");
    m_display.setTextColor(Color::kText, cancelBg);
    m_display.drawText(196, 194, "취소");
}

void MainUiRenderer::renderPreset()
{
    if (m_model.presetConfirm) {
        renderConfirm("프리셋 적용", "정말 다시 생성하겠습니까?", "현재 계획이 새 프리셋으로 바뀝니다.");
        return;
    }
    for (uint8_t i = 0; i < 4; ++i) {
        char no[8];
        std::snprintf(no, sizeof(no), "%u", i + 1U);
        drawRow(64 + i * 32, no, presetName(i), m_model.presetCursor == i, kTeal);
    }
}

void MainUiRenderer::renderPlanList()
{
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(12, 56, "Day   Temp    Humi    Turn    Int");
    for (uint8_t i = 0; i < m_model.planListCount; ++i) {
        const auto& item = m_model.planList[i];
        uint16_t day = item.day;
        int y = 72 + i * 28;
        bool sel = (day == m_model.editDay);
        uint32_t bg = sel ? Color::kSelected : Color::kBg;
        m_display.fillRect(8, y, 304, 26, bg);
        m_display.drawRect(8, y, 304, 26, sel ? kTeal : kPanelSoft);
        char row[64];
        std::snprintf(row, sizeof(row), "D%02u %.1fC %.0f%% %s %um%s",
                      day,
                      item.targetTempC,
                      item.targetHumidPct,
                      item.turningEnabled ? "ON" : "OFF",
                      item.intervalMin,
                      item.overridden ? "*" : "");
        m_display.setTextSize(2);
        m_display.setTextColor(sel ? Color::kText : Color::kTextDim, bg);
        m_display.drawText(14, y + 6, row);
    }
}

void MainUiRenderer::renderPlanEdit()
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "Day %u", m_model.editDay);
    drawRow(54, "일차", buffer, false, kTeal);
    std::snprintf(buffer, sizeof(buffer), "%.1f C", m_model.editTempC);
    drawRow(84, "온도", buffer, m_model.fieldCursor == 0, Color::kAccentTemp);
    std::snprintf(buffer, sizeof(buffer), "%.0f %%", m_model.editHumidPct);
    drawRow(114, "습도", buffer, m_model.fieldCursor == 1, Color::kAccentHumi);
    drawRow(144, "전란", m_model.editTurning ? "ON" : "OFF", m_model.fieldCursor == 2,
            m_model.editTurning ? kOk : Color::kOffIcon);
    std::snprintf(buffer, sizeof(buffer), "%u min", m_model.editIntervalMin);
    drawRow(174, "간격", buffer, m_model.fieldCursor == 3, kTeal);

    uint32_t saveBg = (m_model.fieldCursor == 4U) ? Color::kSelected : kPanel;
    uint32_t cancelBg = (m_model.fieldCursor == 5U) ? Color::kSelected : kPanel;
    m_display.fillRect(68, 198, 78, 16, saveBg);
    m_display.drawRect(68, 198, 78, 16, (m_model.fieldCursor == 4U) ? kOk : kPanelSoft);
    m_display.fillRect(174, 198, 78, 16, cancelBg);
    m_display.drawRect(174, 198, 78, 16, (m_model.fieldCursor == 5U) ? Color::kOffIcon : kPanelSoft);
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kText, saveBg);
    m_display.drawText(90, 201, "저장");
    m_display.setTextColor(Color::kText, cancelBg);
    m_display.drawText(196, 201, "취소");

    if (m_model.editMode && m_model.fieldCursor <= 3U) {
        m_display.drawRect(10, 82 + static_cast<int>(m_model.fieldCursor) * 30, 300, 30, kWarn);
    }
}

void MainUiRenderer::renderManual()
{
    drawRow(62, "히터", m_model.heaterOn ? "ON" : "OFF", m_model.manualCursor == 0,
            m_model.heaterOn ? kOk : Color::kOffIcon);
    drawRow(96, "가습기", m_model.humidifierOn ? "ON" : "OFF", m_model.manualCursor == 1,
            m_model.humidifierOn ? kOk : Color::kOffIcon);
    drawRow(130, "전란", m_model.turnerOn ? "ON" : "OFF", m_model.manualCursor == 2,
            m_model.turnerOn ? kOk : Color::kOffIcon);
    drawRow(164, "팬", m_model.fanOn ? "ON" : "OFF", m_model.manualCursor == 3,
            m_model.fanOn ? kOk : Color::kOffIcon);
}

void MainUiRenderer::renderConfirm(const char*, const char* line1, const char* line2)
{
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(34, 82, line1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(34, 104, line2);
    drawRow(148, "아니오", "취소", m_model.confirmCursor == 0, Color::kOffIcon);
    drawRow(180, "예", "실행", m_model.confirmCursor == 1, kDanger);
}

void MainUiRenderer::renderFactoryReset()
{
    m_display.setTextSize(1);
    m_display.setTextColor(kDanger, Color::kBg);
    m_display.drawText(24, 76, "모든 설정, WiFi, 부화 계획이 삭제됩니다.");
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(24, 100, "실행하려면 버튼을 10초간 계속 누르세요.");
    drawProgressBar(32, 138, 256, 18, m_model.factoryProgressPct, kDanger);
    char pct[16];
    std::snprintf(pct, sizeof(pct), "%u%%", m_model.factoryProgressPct);
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(144, 166, pct);
}

void MainUiRenderer::drawStatusIcons()
{
    m_display.setTextSize(1);
    // 500ms 주기로 토글되는 깜빡임 플래그
    const bool blinkVisible = ((m_renderNowMs / 500U) % 2U) == 0U;

    auto drawFooterPill = [&](int x, int w, const char* label, bool active) {
        // 1. 기본 상태 정의 (구동 안 할 때는 어두운 슬레이트 실버로 은은하게 표현)
        uint32_t border = Color::kOffIcon;
        uint32_t fill   = Color::kFooter; // 하단바 배경색 (0x10A3U)
        uint32_t text   = Color::kTextDim; // 메탈릭 실버 그레이

        // 2. 해당 기능이 구동(Active) 중일 때의 고대비 깜빡임 로직
        if (active) {
            border = kOk; // 테두리는 언제나 선명한 에메랄드 민트 고정
            
            if (blinkVisible) {
                // [ON 타이밍] 배경을 민트색으로 채우고, 글자는 가장 밝은 화이트로 시인성 확보!
                fill = kOk;
                text = Color::kText; // 0xFFFFU (크리스탈 화이트)
            } else {
                // [OFF 타이밍] 배경을 비우고(바탕색), 글자 자체를 민트색으로 변경하여 대비 극대화!
                fill = Color::kFooter;
                text = kOk; // 0x3666U (에메랄드 민트)
            }
        }

        // 3. 정밀하게 계산된 색상으로 렌더링
        m_display.fillRect(x, 218, w, 20, fill);
        m_display.drawRect(x, 218, w, 20, border);
        m_display.setTextColor(text, fill);
        m_display.drawText(x + (hasUtf8(label) ? 8 : 6), 222, label);
    };

    // 하단 제어 상태 알약 라벨 드로잉
    drawFooterPill(8,  42, "히터", m_model.heaterOn);
    drawFooterPill(54, 42, "가습", m_model.humidifierOn);
    drawFooterPill(100, 42, "전란", m_model.turnerOn);
    drawFooterPill(146, 32, "팬",  m_model.fanOn);
}

void MainUiRenderer::drawSignalBars(int x, int y, bool connected, bool configured)
{
    const uint32_t offColor = configured ? kPanelSoft : Color::kOffIcon;
    for (uint8_t i = 0; i < 4; ++i) {
        const int h = 4 + static_cast<int>(i) * 3;
        const uint32_t color = connected ? kOk : (configured && i == 0U ? kWarn : offColor);
        m_display.fillRect(x + static_cast<int>(i) * 5, y + 14 - h, 3, h, color);
    }
}

void MainUiRenderer::drawProgressBar(int x, int y, int w, int h, uint8_t pct, uint32_t color)
{
    if (pct > 100U) pct = 100U;
    int fillW = (w - 4) * static_cast<int>(pct) / 100;
    m_display.drawRect(x, y, w, h, kPanelSoft);
    m_display.fillRect(x + 2, y + 2, w - 4, h - 4, 0x0000U);
    if (fillW > 0) m_display.fillRect(x + 2, y + 2, fillW, h - 4, color);
}

void MainUiRenderer::drawPill(int x, int y, int w, const char* label, uint32_t color)
{
    m_display.drawRect(x, y, w, 20, color);
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(x + (hasUtf8(label) ? 8 : 6), y + 4, label);
}

void MainUiRenderer::drawRow(int y, const char* label, const char* value, bool selected, uint32_t accent)
{
    uint32_t bg = selected ? Color::kSelected : kPanel;
    if (accent == 0) accent = Color::kTextDim;
    m_display.fillRect(12, y, 296, 28, bg);
    m_display.drawRect(12, y, 296, 28, selected ? accent : kPanelSoft);
    m_display.fillRect(18, y + 7, 6, 14, accent);
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kTextDim, bg);
    m_display.drawText(32, y + 5, label);
    bool compact = hasUtf8(value) || std::strlen(value) > 9U;
    m_display.setTextSize(compact ? 1 : 2);
    m_display.setTextColor(Color::kText, bg);
    m_display.drawText(compact ? 178 : 208, compact ? y + 10 : y + 7, value);
}

} // namespace incubator::ui
