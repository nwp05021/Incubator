#include "ui/MainUiRenderer.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace incubator::ui
{
namespace
{
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
        if (model.screen == UiScreen::AwsTest) return "AWS 테스트";
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

        // MainUiRenderer.cpp 내부 getPageHash() 함수 하단
        hashValue(hash, model.awsTestRunning);
        hashValue(hash, model.awsEndpointCursor);
        hashValue(hash, model.awsTestCount);
        hashValue(hash, model.awsLastResult);
        for (uint8_t i = 0; i < 4; ++i) {
            hashString(hash, model.awsTestLog[i]);
        }
        return hash;        
    }
} // namespace

void MainUiRenderer::render(uint32_t nowMs)
{
    // 30ms 렌더링 주기 제한 케어
    if (m_hasRendered && nowMs - m_lastRenderMs < 30U) return;

    // 1. 각 독립 영역의 변경 사항(Dirty) 상태를 먼저 계산합니다.
    const uint32_t currentHeaderHash = getHeaderHash(m_model, nowMs);
    const uint32_t currentPageHash   = getPageHash(m_model, nowMs);
    const uint32_t currentFooterHash = getFooterHash(m_model, nowMs);

    bool isHeaderDirty = m_firstRender || (currentHeaderHash != m_lastHeaderHash);
    bool isPageDirty   = m_firstRender || (currentPageHash != m_lastPageHash);
    bool isFooterDirty = m_firstRender || (currentFooterHash != m_lastFooterHash);

    // [버그 수정] 상단바 시간(초)이 바뀔 때마다 본문 전체를 fillRect로 지우고 
    // 새로 그리던 강제 연동 로직을 과감히 제거합니다.
    // if (isHeaderDirty) { isPageDirty = true; }

    bool isAnyDirty = isHeaderDirty || isPageDirty || isFooterDirty;

    // 2. 만약 변경사항이 단 하나도 없다면 SPI 버스 과부하 및 화면 깜박임을 
    // 완전히 차단하기 위해 아무 작업도 하지 않고 루프를 탈출(Early Return)합니다.
    if (!isAnyDirty) return;

    // 타임스탬프 및 내부 상태 최신화
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

    m_lastHeaderHash = currentHeaderHash;
    m_lastPageHash   = currentPageHash;
    m_lastFooterHash = currentFooterHash;
    m_firstRender    = false;
    m_hasRendered    = true;

    // 3. 진짜 변경이 일어난 영역만 부분적으로 캔버스(또는 화면)에 렌더링합니다.
    // 1) 상단바 영역
    if (isHeaderDirty) {
        drawStatusBar(nowMs);
    }

    // 2) 본문 바디 영역 (값의 변화나 페이지 전환 시에만 동작)
    if (isPageDirty) {
        m_display.fillRect(0, 30, Layout::kScreenW, 185, Color::kBg);

        switch (m_model.screen) {
            case UiScreen::Main:
                if (m_model.activePage == 0) m_pageMain.render(nowMs);
                else if (m_model.activePage == 1) renderPage1();
                else m_pageHelp.render(nowMs);
                break;
            case UiScreen::Menu: m_pageMenu.render(nowMs); break;
            case UiScreen::StartDate: m_pageStartDate.render(nowMs); break;
            case UiScreen::Preset: m_pagePreset.render(nowMs); break;
            case UiScreen::PlanList: m_pagePlanList.render(nowMs); break;
            case UiScreen::PlanEdit: m_pagePlanEdit.render(nowMs); break;
            case UiScreen::Manual: m_pageManual.render(nowMs); break;
            case UiScreen::AwsTest: m_pageAwsTest.render(nowMs); break;
            case UiScreen::WifiReset:
                renderConfirm("WiFi 정보 리셋", "저장된 WiFi 인증정보를 삭제합니다.", "삭제 후 BLE 설정이 필요합니다.");
                break;
            case UiScreen::RebootConfirm:
                renderConfirm("시스템 재부팅", "장치를 다시 시작합니다.", "진행 중 제어가 잠시 중단됩니다.");
                break;
            case UiScreen::FactoryReset: m_pageFactoryReset.render(nowMs); break;
            case UiScreen::System: m_pageAwsTest.render(nowMs); break;

            // 🔥 이 한 줄을 추가해 주세요!
            case UiScreen::BleProvisioning: break;            
        }

        if (m_model.safeMode && m_model.screen == UiScreen::Main) {
            m_display.fillRect(72, 102, 176, 34, Color::kDanger);
            m_display.setTextSize(2);
            m_display.setTextColor(Color::kText, Color::kDanger);
            m_display.drawText(104, 112, "SAFE MODE");
        }
    }

    // 3) 하단바 영역
    if (isFooterDirty) {
        renderFooter(nowMs);
    }

    // 실제로 변경이 발생했을 때만 최종 버퍼를 전송합니다.
    m_display.endFrame();
}

void MainUiRenderer::drawStatusBar(uint32_t nowMs)
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
        const uint32_t warnColor = hardAlarm ? Color::kDanger : (sensorWarning ? Color::kWarn : Color::kOffIcon);
        m_display.drawRect(292, 6, 18, 14, warnColor);
        m_display.setTextSize(1);
        m_display.setTextColor(warnColor, Color::kHeader);
        m_display.drawText(298, 7, "!");
    }
    m_display.setTextSize(1);
    m_display.drawLine(0, 25, Layout::kScreenW, 25, Color::kDivider);
}

void MainUiRenderer::renderHeader(uint32_t nowMs)
{
    m_display.fillRect(0, 24, Layout::kScreenW, 26, Color::kHeader);
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kText, Color::kHeader);
    m_display.drawText(12, 29, screenTitle(m_model));
    m_display.setTextColor(Color::kTextDim, Color::kHeader);
    m_display.drawText(244, 33, "2026-05-05");
    m_display.drawLine(0, 49, Layout::kScreenW, 49, Color::kDivider);
}

void MainUiRenderer::renderFooter(uint32_t nowMs)
{
    m_display.drawLine(0, 215, Layout::kScreenW, 215, Color::kDivider);
    m_display.fillRect(0, 216, Layout::kScreenW, 24, Color::kFooter);
    
    if (m_model.screen == UiScreen::Main) {
        char progress[20];
        drawStatusIcons(nowMs); 
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
    m_display.setTextColor(isLock ? Color::kWarn : (isTurnOn ? Color::kOnIcon : Color::kOffIcon), Color::kBg);
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

void MainUiRenderer::renderConfirm(const char* title, const char* line1, const char* line2)
{
m_display.setTextSize(1);
    
    // 1. 타이틀 출력 (필요한 경우 추가)
    m_display.setTextColor(Color::kAccentTemp, Color::kBg); // 눈에 띄는 색상 적용
    m_display.drawText(34, 60, title); 
    
    // 2. 안내 문구 출력
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(34, 82, line1);
    
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(34, 104, line2);
    
    // 3. 확인 버튼 출력
    drawRow(148, "아니오", "취소", m_model.confirmCursor == 0, Color::kOffIcon);
    drawRow(180, "예", "실행", m_model.confirmCursor == 1, Color::kDanger);
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
    m_display.drawRect(x, y, w, h, Color::kPanelSoft);
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
    m_display.drawRect(x, y, w, h, Color::kPanelSoft);
    m_display.setTextSize(1);
    m_display.drawText(x + (w / 2) - (std::strlen(label) * 3), y + (h / 2) - 4, label);
}

void MainUiRenderer::drawRow(int y, const char* label, const char* value, bool selected, uint32_t valColor, int valSize)
{
    uint32_t bg     = selected ? Color::kText : Color::kPanel;
    uint32_t border = selected ? Color::kText : Color::kPanelSoft;
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
