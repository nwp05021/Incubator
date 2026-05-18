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
    static constexpr uint32_t kPanel      = 0x2146U; 
    static constexpr uint32_t kPanelSoft  = 0x31CDU; 
    static constexpr uint32_t kOk         = 0x3666U; 
    static constexpr uint32_t kWarn       = 0xECE4U; 
    static constexpr uint32_t kDanger     = 0xD906U; 
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
        std::snprintf(out, len, "%02u:%02u:%02u",
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
        
        hashValue(hash, model.heaterOn);
        hashValue(hash, model.humidifierOn);
        hashValue(hash, model.turnerOn);
        hashValue(hash, model.fanOn);

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
        
        if (model.provisioningActive) {
            hashValue(hash, model.provisioningRemainingMs / 1000U);
            hashString(hash, model.provisioningName);
            hashString(hash, model.provisioningPop);
            hashString(hash, model.provisioningMessage);
        }

        if (model.screen == UiScreen::System) {
            hashValue(hash, model.bootCount);
            hashValue(hash, model.batchActive);
            hashString(hash, model.ipAddress);
        }
        return hash;
    }
} // namespace

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

    // [핵심 수정] WiFi 연결 상태나 알람 상태가 변경되어 상단바가 갱신되어야 한다면 
    // 본문 페이지도 함께 dirty 상태로 만들어 전체적인 렌더링 싱크를 맞추고 잔상을 방지합니다.
    if (isHeaderDirty) {
        isPageDirty = true;
    }

    m_lastHeaderHash = currentHeaderHash;
    m_lastPageHash   = currentPageHash;
    m_lastFooterHash = currentFooterHash;
    m_firstRender    = false;
    m_hasRendered    = true;

    // 1. 상단바 그리기
    if (isHeaderDirty) {
        drawStatusBar();
    }

    // 2. 본문 바디 영역 그리기
    if (isPageDirty) {
        // [핵심 수정] 기존에 26부터 지우던 것을 상단바 영역(0~29)을 완벽히 보존하기 위해 
        // 확실하게 Layout::kBodyY (50) 또는 상단바 경계 바깥인 30부터 지우도록 수정합니다.
        m_display.fillRect(0, 30, Layout::kScreenW, 185, Color::kBg);

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

        if (m_model.safeMode && m_model.screen == UiScreen::Main) {
            m_display.fillRect(72, 102, 176, 34, kDanger);
            m_display.setTextSize(2);
            m_display.setTextColor(Color::kText, kDanger);
            m_display.drawText(104, 112, "SAFE MODE");
        }
    }

    // 3. 하단바 그리기
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
        drawStatusIcons(); 
        std::snprintf(progress, sizeof(progress), "%u일차/%u", m_model.currentDay, m_model.totalDays);
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kText, Color::kFooter);
        m_display.drawText(244, 223, progress); 
    } else {
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kFooter);
        const char* hint = "회전: 이동  클릭: 선택  길게: 뒤로";
        if (m_model.screen == UiScreen::Manual) hint = "회전: 선택  클릭: 토글  길게: 종료";
        if (m_model.screen == UiScreen::PlanEdit) hint = "회전: 조절  클릭: 다음  길게: 저장";
        if (m_model.screen == UiScreen::FactoryReset) hint = "버튼 10초 유지: 실행  길게: 뒤로";
        m_display.drawText(12, 223, hint);
    }
}

void MainUiRenderer::renderPage0()
{
    char buffer[48];

    m_display.drawLine(160, 36, 160, 154, Color::kDivider);
    m_display.drawLine(20, 146, 140, 146, kPanelSoft);
    m_display.drawLine(180, 146, 300, 146, kPanelSoft);

    m_display.setTextSize(1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(22, 42, "온도"); 

    if (m_model.tempSensorFault) {
        m_display.setTextSize(1);
        m_display.setTextColor(kDanger, Color::kBg);
        m_display.drawText(54, 74, "---");
        m_display.drawText(56, 122, "FAULT");
    } else {
        m_display.setTextSize(1); 
        m_display.setTextColor(Color::kAccentTemp, Color::kBg);
        std::snprintf(buffer, sizeof(buffer), "%.1f", m_model.displayTempC);
        m_display.drawNumberText(24, 74, buffer); 

        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(140, 76, "C");
    }

    m_display.setTextSize(1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(182, 42, "습도");

    if (m_model.humiSensorFault) {
        m_display.setTextSize(1);
        m_display.setTextColor(kDanger, Color::kBg);
        m_display.drawText(214, 74, "---");
        m_display.drawText(216, 122, "FAULT");
    } else {
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kAccentHumi, Color::kBg);
        std::snprintf(buffer, sizeof(buffer), "%.0f", m_model.displayHumidPct);
        m_display.drawNumberText(198, 74, buffer);

        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(274, 76, "%");
    }

    m_display.setTextSize(1);
    std::snprintf(buffer, sizeof(buffer), "SET %.1f C", m_model.targetTempC);
    drawPill(20, 172, 120, buffer, Color::kAccentTemp); 

    std::snprintf(buffer, sizeof(buffer), "SET %.0f %%", m_model.targetHumidPct);
    drawPill(180, 172, 120, buffer, Color::kAccentHumi);
}

void MainUiRenderer::renderPage1()
{
    char buffer[64];
    // Layout::kBodyY(50)를 기준으로 줄간격을 정렬합니다.
    const int startY = Layout::kBodyY + 12;   
    const int lineGap = 34;  

    m_display.setTextSize(1);

    // --- 1행: 목표 온도 및 Hysteresis ---
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY, "목표 온도");
    
    std::snprintf(buffer, sizeof(buffer), "%.1f C", m_model.targetTempC);
    m_display.setTextColor(Color::kAccentTemp, Color::kBg);
    m_display.drawText(100, startY, buffer);

    m_display.setTextColor(Color::kTextDim, Color::kBg);
    // UiModel에 변수가 없으므로 레이아웃 기획서 요구사항(0.3) 수치 투영
    m_display.drawText(200, startY, "Hyst:   0.3"); 


    // --- 2행: 목표 습도 및 Hysteresis ---
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY + lineGap, "목표 습도");
    
    std::snprintf(buffer, sizeof(buffer), "%.0f %%", m_model.targetHumidPct);
    m_display.setTextColor(Color::kAccentHumi, Color::kBg);
    m_display.drawText(100, startY + lineGap, buffer);

    m_display.setTextColor(Color::kTextDim, Color::kBg);
    // UiModel에 변수가 없으므로 레이아웃 기획서 요구사항(2%) 수치 투영
    m_display.drawText(200, startY + lineGap, "Hyst:   2%");


    // --- 3행: 전란 상태 및 전란 간격 ---
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY + (lineGap * 2), "전란");
    
    const bool isLock = m_model.lockdownActive;
    const bool isTurnOn = m_model.turningEnabled;
    m_display.setTextColor(isLock ? kWarn : (isTurnOn ? Color::kOnIcon : Color::kOffIcon), Color::kBg);
    m_display.drawText(100, startY + (lineGap * 2), isLock ? "LOCK" : (isTurnOn ? "ON" : "OFF"));

    m_display.setTextColor(Color::kTextDim, Color::kBg);
    // 모델의 기본 설정값 정보(editIntervalMin)를 기반으로 기획서의 '전란 간격 xxmin' 양식 출력
    std::snprintf(buffer, sizeof(buffer), "전란 간격 %umin", m_model.editIntervalMin);
    m_display.drawText(180, startY + (lineGap * 2), buffer);


    // --- 4행: 다음 전란 남은 시간 ---
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY + (lineGap * 3), "다음 전란");
    
    m_display.setTextColor(Color::kText, Color::kBg);
    std::snprintf(buffer, sizeof(buffer), "%u분 후", m_model.nextTurningInMin);
    m_display.drawText(100, startY + (lineGap * 3), buffer);
}

void MainUiRenderer::renderHelp()
{
    const int startY = Layout::kBodyY + 8;
    const int lineGap = 24;

    m_display.setTextSize(1);
    
    // 프리미엄 가전 테마 텍스트 컬러 매핑 적용
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(16, startY, "기본 메뉴 작동법");

    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(16, startY + lineGap,       " 휠 회전  : 메인 화면 전환 및 메뉴 커서 이동");
    m_display.drawText(16, startY + (lineGap * 2), " 버튼 클릭: 항목 선택 및 데이터 편집 모드 진입");
    m_display.drawText(16, startY + (lineGap * 3), " 버튼 길게: 설정 메뉴 진입 및 이전 화면 복귀");
    
    m_display.setTextColor(kWarn, Color::kBg);
    m_display.drawText(16, startY + (lineGap * 4), "※ 수동 테스트 제어는 확인 후 자동 복귀됩니다.");
    
    m_display.setTextColor(Color::kAccentHumi, Color::kBg);
    m_display.drawText(16, startY + (lineGap * 5), "※ 네트워크 연결은 BLE 모바일 설정을 지원합니다.");
}

void MainUiRenderer::renderPage2() { renderManual(); }
void MainUiRenderer::renderPage3() { renderPlanEdit(); }

void MainUiRenderer::renderPage4()
{
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%u s", (unsigned int)(m_model.uptimeMs / 1000U));
    drawRow(58, "Uptime", buffer, false, Color::kText);
    std::snprintf(buffer, sizeof(buffer), "%u", (unsigned int)m_model.bootCount);
    drawRow(84, "Boot cnt", buffer, false, Color::kTextDim);
    drawRow(110, "Cloud", m_model.cloudConnected ? "ON" : "OFF", false, m_model.cloudConnected ? Color::kOnIcon : Color::kOffIcon);
    drawRow(136, "IP", m_model.ipAddress[0] ? m_model.ipAddress : "IP 없음", false, Color::kTextDim);
    drawRow(162, "Batch", m_model.batchActive ? "ACTIVE" : "STOP", false, m_model.batchActive ? Color::kOnIcon : Color::kOffIcon);
    drawRow(188, "SafeMode", m_model.safeMode ? "YES" : "NO", false, m_model.safeMode ? kDanger : Color::kOnIcon);
}

void MainUiRenderer::renderMenu()
{
    static constexpr const char* kNames[kMenuCount] = {
        "부화 시작일", "프리셋 선택", "일별 설정", "수동 테스트",
        "WiFi 리셋", "BLE 설정", "재부팅", "공장 초기화"
    };
    static constexpr const char* kDesc[kMenuCount] = {
        "날짜 관리", "종 선택", "스케줄 표", "배선 점검",
        "인증 삭제", "QR 연결", "시스템", "10초 유지"
    };

    constexpr uint8_t kMaxVisible = 5;
    uint8_t topIndex = 0;
    
    if (m_model.menuCursor >= kMaxVisible) {
        topIndex = m_model.menuCursor - kMaxVisible + 1;
    }

    // [수정 완료] 타이핑 오타 수식 (v < v < kMaxVisible) -> (v < kMaxVisible)로 정상화
    for (uint8_t v = 0; v < kMaxVisible; ++v) {
        uint8_t i = topIndex + v;
        if (i >= kMenuCount) break; 

        int y = 54 + static_cast<int>(v) * 31; 
        bool selected = (m_model.menuCursor == i);

        uint32_t bg     = selected ? Color::kText : kPanel; 
        uint32_t border = selected ? Color::kText : kPanelSoft;
        uint32_t textFg = selected ? Color::kBg   : Color::kText; 
        uint32_t descFg = selected ? Color::kBg   : Color::kTextDim; 

        m_display.fillRect(12, y, 296, 27, bg);
        m_display.drawRect(12, y, 296, 27, border);

        m_display.setTextSize(1);
        m_display.setTextColor(textFg, bg);
        m_display.drawText(24, y + 7, kNames[i]);

        m_display.setTextColor(descFg, bg);
        m_display.drawText(196, y + 7, kDesc[i]);
    }
}

void MainUiRenderer::renderStartDate()
{
    char dateStr[8];
    static constexpr const char* labels[] = { "년", "월", "일" };
    int values[] = { m_model.editBatchYear, m_model.editBatchMonth, m_model.editBatchDay };

    for (int i = 0; i < 3; ++i) {
        int x = 16 + i * 98;
        int y = 60; 
        
        bool fieldSel = (m_model.screen == UiScreen::StartDate && m_model.fieldCursor == i);
        
        uint32_t valBg = fieldSel ? Color::kText : kPanel;
        uint32_t valFg = fieldSel ? Color::kBg   : Color::kText; 
        uint32_t boxBorder = fieldSel ? Color::kText : kPanelSoft;

        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(x + 36, y + 6, labels[i]); 

        m_display.fillRect(x, y + 20, 84, 56, valBg);
        m_display.drawRect(x, y + 20, 84, 56, boxBorder);
        
        if (i == 0) std::snprintf(dateStr, sizeof(dateStr), "%04d", values[i]); 
        else        std::snprintf(dateStr, sizeof(dateStr), "%02d", values[i]); 
        
        m_display.setTextSize(2);
        m_display.setTextColor(valFg, valBg);
        
        int fontX = (i == 0) ? x + 18 : x + 30;
        int fontY = y + 40;
        m_display.drawText(fontX, fontY, dateStr);
    }

    const uint32_t saveBg = (m_model.fieldCursor == 3U) ? Color::kText : kPanel;
    const uint32_t saveFg = (m_model.fieldCursor == 3U) ? Color::kBg   : Color::kText; 

    m_display.setTextSize(1);
    m_display.setTextColor(saveFg, saveBg);
    drawButton(20, 164, 130, 32, "등 록", saveBg); 

    const uint32_t cancelBg = (m_model.fieldCursor == 4U) ? Color::kText : kPanel;
    const uint32_t cancelFg = (m_model.fieldCursor == 4U) ? Color::kBg   : Color::kText; 

    m_display.setTextColor(cancelFg, cancelBg);
    drawButton(170, 164, 130, 32, "취 소", cancelBg);
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
        drawRow(56 + i * 36, no, presetName(i), m_model.presetCursor == i, Color::kText, 1);
    }
}

void MainUiRenderer::renderPlanList()
{
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(12, 52, "Day   Temp    Humi    Turn    Int");
    
    for (uint8_t i = 0; i < m_model.planListCount; ++i) {
        const auto& item = m_model.planList[i];
        uint16_t day = item.day;
        int y = 68 + i * 28;
        
        bool sel = (day == m_model.editDay);
        
        uint32_t bg     = sel ? Color::kText : kPanel;
        uint32_t border = sel ? Color::kText : kPanelSoft;
        uint32_t textFg = sel ? Color::kBg   : Color::kText;

        m_display.fillRect(8, y, 304, 26, bg);
        m_display.drawRect(8, y, 304, 26, border);
        
        char row[64];
        std::snprintf(row, sizeof(row), "D%02u  %.1f C  %.0f %%   %s   %u분%s",
                      day,
                      item.targetTempC,
                      item.targetHumidPct,
                      item.turningEnabled ? "ON" : "OFF",
                      item.intervalMin,
                      item.overridden ? "*" : "");
        
        m_display.setTextSize(1); 
        m_display.setTextColor(textFg, bg);
        m_display.drawText(16, y + 9, row);
    }
}

void MainUiRenderer::renderPlanEdit()
{
    char buffer[32];

    m_display.setTextSize(1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    std::snprintf(buffer, sizeof(buffer), "D%02u 일정 상세 편집", m_model.editDay);
    m_display.drawText(16, 34, buffer);
    
    std::snprintf(buffer, sizeof(buffer), "%.1f C", m_model.editTempC);
    drawRow(54, "목표 온도", buffer, m_model.fieldCursor == 0, Color::kText, 1);
    
    std::snprintf(buffer, sizeof(buffer), "%.0f %%", m_model.editHumidPct);
    drawRow(84, "목표 습도", buffer, m_model.fieldCursor == 1, Color::kText, 1);
    
    drawRow(114, "전란 모드", m_model.editTurning ? "ON" : "OFF", m_model.fieldCursor == 2, m_model.editTurning ? Color::kOnIcon : Color::kOffIcon, 1);
    
    std::snprintf(buffer, sizeof(buffer), "%u 분", m_model.editIntervalMin);
    drawRow(144, "전란 주기", buffer, m_model.fieldCursor == 3, Color::kText, 1);

    uint32_t saveBg = (m_model.fieldCursor == 4U) ? Color::kText : kPanel;
    uint32_t saveFg = (m_model.fieldCursor == 4U) ? Color::kBg   : Color::kText; 
    m_display.setTextColor(saveFg, saveBg);
    drawButton(20, 180, 130, 28, "저 장", saveBg);

    uint32_t cancelBg = (m_model.fieldCursor == 5U) ? Color::kText : kPanel;
    uint32_t cancelFg = (m_model.fieldCursor == 5U) ? Color::kBg   : Color::kText; 
    m_display.setTextColor(cancelFg, cancelBg);
    drawButton(170, 180, 130, 28, "취 소", cancelBg);
}

void MainUiRenderer::renderManual()
{
    drawRow(62, "히터 제어", m_model.heaterOn ? "ON" : "OFF", m_model.manualCursor == 0,
            m_model.heaterOn ? Color::kOnIcon : Color::kOffIcon, 2);
    drawRow(96, "가습 제어", m_model.humidifierOn ? "ON" : "OFF", m_model.manualCursor == 1,
            m_model.humidifierOn ? Color::kOnIcon : Color::kOffIcon, 2);
    drawRow(130, "전란 모터", m_model.turnerOn ? "ON" : "OFF", m_model.manualCursor == 2,
            m_model.turnerOn ? Color::kOnIcon : Color::kOffIcon, 2);
    drawRow(164, "순환 팬", m_model.fanOn ? "ON" : "OFF", m_model.manualCursor == 3,
            m_model.fanOn ? Color::kOnIcon : Color::kOffIcon, 2);
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
    const bool blinkVisible = ((m_renderNowMs / 500U) % 2U) == 0U;

    auto drawFooterPill = [&](int x, int w, const char* label, bool active) {
        uint32_t border = Color::kOffIcon;
        uint32_t fill   = Color::kFooter; 
        uint32_t text   = Color::kTextDim; 

        if (active) {
            border = Color::kOnIcon; 
            if (blinkVisible) {
                fill = Color::kOnIcon;
                text = Color::kText; 
            } else {
                fill = Color::kFooter;
                text = Color::kOnIcon; 
            }
        }

        m_display.fillRect(x, 218, w, 20, fill);
        m_display.drawRect(x, 218, w, 20, border);
        m_display.setTextColor(text, fill);
        m_display.drawText(x + (hasUtf8(label) ? 8 : 6), 222, label);
    };

    drawFooterPill(8,  42, "히터", m_model.heaterOn);
    drawFooterPill(54, 42, "가습", m_model.humidifierOn);
    drawFooterPill(100, 42, "전란", m_model.turnerOn);
    drawFooterPill(146, 32, "팬",  m_model.fanOn);
}

void MainUiRenderer::drawSignalBars(int x, int y, bool connected, bool configured)
{
    // [수정] 지나치게 어두웠던 kOffIcon/kPanelSoft 대신 가독성이 검증된 kTextDim(실버)을 베이스로 사용합니다.
    const uint32_t offColor = Color::kTextDim; 
    
    for (uint8_t i = 0; i < 4; ++i) {
        const int h = 4 + static_cast<int>(i) * 3;
        
        // 연결 완료 시 그린(kOnIcon), 설정되었으나 연결 시도 중일 때 첫 칸은 옐로우(kWarn), 나머지는 실버(offColor)
        const uint32_t color = connected ? Color::kOnIcon : offColor;
        
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

void MainUiRenderer::drawButton(int x, int y, int w, int h, const char* label, uint32_t bgColor)
{
    m_display.fillRect(x, y, w, h, bgColor);
    m_display.drawRect(x, y, w, h, kPanelSoft);
    m_display.setTextSize(1);
    m_display.drawText(x + (w / 2) - (std::strlen(label) * 3), y + (h / 2) - 4, label);
}

void MainUiRenderer::drawRow(int y, const char* label, const char* value, bool selected, uint32_t valColor, int valSize)
{
    uint32_t bg     = selected ? Color::kText : kPanel;
    uint32_t border = selected ? Color::kText : kPanelSoft;
    uint32_t textFg = selected ? Color::kBg   : Color::kText; 

    m_display.fillRect(12, y, 296, 27, bg);
    m_display.drawRect(12, y, 296, 27, border);

    m_display.setTextSize(1);
    m_display.setTextColor(textFg, bg);
    m_display.drawText(20, y + 7, label); 

    uint32_t vFg = selected ? Color::kBg : valColor;
    m_display.setTextColor(vFg, bg);
    
    m_display.setTextSize(valSize);
    if (valSize == 2) {
        m_display.drawText(270 - (std::strlen(value) * 12), y + 6, value);
    } else {
        m_display.drawText(270 - (std::strlen(value) * 6), y + 7, value);
    }
}

} // namespace incubator::ui