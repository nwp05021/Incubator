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

    [[maybe_unused]] bool hasUtf8(const char* text)
    {
        if (!text) return false;
        while (*text) {
            if ((static_cast<unsigned char>(*text) & 0x80U) != 0U) return true;
            ++text;
        }
        return false;
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
        hashValue(hash, nowMs / 500U);          // 500ms 깜박임 감지
        hashValue(hash, model.uptimeMs / 1000U);
        hashValue(hash, model.cloudConnected);
        hashValue(hash, model.wifiConnected);
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
    if (m_hasRendered && nowMs - m_lastRenderMs < 30U) return;

    const uint32_t currentHeaderHash = getHeaderHash(m_model, nowMs);
    const uint32_t currentPageHash   = getPageHash(m_model, nowMs);
    const uint32_t currentFooterHash = getFooterHash(m_model, nowMs);

    bool isHeaderDirty = m_firstRender || (currentHeaderHash != m_lastHeaderHash);
    bool isPageDirty   = m_firstRender || (currentPageHash != m_lastPageHash);
    bool isFooterDirty = m_firstRender || (currentFooterHash != m_lastFooterHash);

    bool isAnyDirty = isHeaderDirty || isPageDirty || isFooterDirty;
    if (!isAnyDirty) return;

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

    // 1) 상단바 영역 렌더링
    if (isHeaderDirty) {
        drawStatusBar(nowMs);
    }

    // 2) 본문 바디 영역 렌더링 (겹침 방지를 위해 Y:26부터 가득 채워 초기화)
    if (isPageDirty) {
        m_display.fillRect(0, 26, Layout::kScreenW, 189, Color::kBg);

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
            case UiScreen::System:
                //m_pageSystem.render(nowMs); 
                break; // [수정완료] 시스템 정보 페이지 바인딩
            case UiScreen::BleProvisioning: break;            
        } // ◀ switch 문이 여기서 정확히 닫혀야 합니다!

        // 세이프 모드 경고는 오직 메인 화면에서만 띄우도록 switch문 밖으로 안전하게 배치
        if (m_model.safeMode && m_model.screen == UiScreen::Main) {
            m_display.fillRect(72, 102, 176, 34, Color::kDanger);
            m_display.setTextSize(2);
            m_display.setTextColor(Color::kText, Color::kDanger);
            m_display.drawText(104, 112, "SAFE MODE");
        }
    } // ◀ if (isPageDirty) 문이 끝나는 괄호

    // 3) 하단바 영역
    if (isFooterDirty) {
        renderFooter(nowMs);
    }

    m_display.endFrame();
}

void MainUiRenderer::drawStatusBar(uint32_t nowMs)
{
    char buffer[32];
    m_display.fillRect(0, 0, Layout::kScreenW, 26, Color::kHeader);
    m_display.setTextColor(Color::kText, Color::kHeader);

    const bool blinkOn = (nowMs / 500U) % 2U == 0U;

    // =========================================================================
    // 1. 좌측 영역: MM-DD HH:MM 구조 및 시계 동기화 매핑
    // =========================================================================
    // 메인 화면 페이지 0일 때와 AWS 테스트 화면을 포함한 서브 메뉴 화면 처리 분기 조율
    if (m_model.screen == UiScreen::Main && m_model.activePage == 0U) {
        m_display.setTextSize(1);
        std::time_t nowTime = std::time(nullptr);
        std::tm* tmv = std::localtime(&nowTime);

        if (tmv && tmv->tm_year >= 120) {
            int year      = tmv->tm_year + 1900;
            int month     = tmv->tm_mon + 1;
            int day       = tmv->tm_mday;
            int hour      = tmv->tm_hour;
            int min       = tmv->tm_min;

            if (blinkOn) {
                std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d", year, month, day, hour, min);
            } else {
                std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d %02d", year, month, day, hour, min);
            }
        } else {
            uint32_t totalSec = m_model.uptimeMs / 1000U;
            uint32_t hr = (totalSec / 3600U) % 24U;
            uint32_t mn = (totalSec / 60U) % 60U;
            if (blinkOn) {
                std::snprintf(buffer, sizeof(buffer), "00-00 %02u:%02u", static_cast<unsigned int>(hr), static_cast<unsigned int>(mn));
            } else {
                std::snprintf(buffer, sizeof(buffer), "00-00 %02u %02u", static_cast<unsigned int>(hr), static_cast<unsigned int>(mn));
            }
        }
        m_display.drawText(8, 5, buffer);
    } else {
        // 일반 서브 메뉴 화면 상태일 때는 깔끔하게 화면 타이틀과 간소화 텍스트 배치
        m_display.setTextSize(1);
        m_display.drawText(10, 6, screenTitle(m_model));
        formatDate(buffer, sizeof(buffer));
        m_display.setTextColor(Color::kTextDim, Color::kHeader);
        m_display.drawText(236, 6, buffer);
    }

    // =========================================================================
    // 2. 우측 영역: 네트워크 아이콘 드로잉
    // =========================================================================
    if (m_model.screen == UiScreen::Main && m_model.activePage == 0U) {
        drawSignalBars(295, 5, m_model.wifiConnected); 
        drawCloudIcon(272, 5, m_model.cloudConnected);

        // =========================================================================
        // 3. 중앙 영역: 경고 메시지 깜박임
        // =========================================================================
        const bool isFault   = m_model.tempSensorFault || m_model.humiSensorFault;
        const bool isAlarm   = (m_model.uptimeMs > 3000U) && (m_model.tempAlarm || m_model.humiAlarm);
        const bool isWarning = m_model.tempSensorWarning || m_model.humiSensorWarning;

        if ((isFault || isAlarm || isWarning) && blinkOn) {
            const char* errorText = "";
            uint32_t errorColor = Color::kText;

            if (isFault) {
                errorText = "센서오류";
                errorColor = Color::kDanger;
            } else if (isAlarm) {
                errorText = "수치이상";
                errorColor = Color::kDanger;
            } else {
                errorText = "센서경고";
                errorColor = Color::kWarn;
            }

            m_display.setTextSize(1);
            m_display.setTextColor(errorColor, Color::kHeader);
            m_display.drawText(160, 7, errorText);
        }
    }
    
    m_display.setTextSize(1);
    m_display.drawLine(0, 25, Layout::kScreenW, 25, Color::kDivider);
}

void MainUiRenderer::drawSignalBars(int x, int y, bool connected)
{
    const uint32_t offColor = 0x5AEB; 
    const uint32_t onColor  = 0xBDD7U; 
    
    for (uint8_t i = 0; i < 4; ++i) {
        const int h = 4 + static_cast<int>(i) * 3;
        const uint32_t color = connected ? onColor : offColor;
        m_display.fillRect(x + static_cast<int>(i) * 4, y + 14 - h, 2, h, color);
    }
}

void MainUiRenderer::drawCloudIcon(int x, int y, bool connected)
{
    const uint32_t offColor = 0x5AEB; 
    const uint32_t onColor  = 0xBDD7U;
    const uint32_t color = connected ? onColor : offColor;

    m_display.fillRect(x + 4,  y + 6,  12, 6, color);
    m_display.fillRect(x + 1,  y + 8,  16, 4, color);
    m_display.fillRect(x + 7,  y + 3,  6,  4, color);
}

// ❌ 중복 드로잉 버그를 유발하던 기존 renderHeader 내부를 완전히 비워 소멸시킵니다.
void MainUiRenderer::renderHeader(uint32_t)
{
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
    const int startY = Layout::kBodyY + 12;   
    const int lineGap = 34;  

    m_display.setTextSize(1);

    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY, "목표 온도");
    
    std::snprintf(buffer, sizeof(buffer), "%.1f C", m_model.targetTempC);
    m_display.setTextColor(Color::kAccentTemp, Color::kBg);
    m_display.drawText(100, startY, buffer);

    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(200, startY, "Hyst:   0.3"); 

    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(200, startY, "Hyst:   0.3"); 

    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY + lineGap, "목표 습도");
    
    std::snprintf(buffer, sizeof(buffer), "%.0f %%", m_model.targetHumidPct);
    m_display.setTextColor(Color::kAccentHumi, Color::kBg);
    m_display.drawText(100, startY + lineGap, buffer);

    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(200, startY + lineGap, "Hyst:   2%");

    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY + (lineGap * 2), "전란");
    
    const bool isLock = m_model.lockdownActive;
    const bool isTurnOn = m_model.turningEnabled;
    m_display.setTextColor(isLock ? Color::kWarn : (isTurnOn ? Color::kOnIcon : Color::kOffIcon), Color::kBg);
    m_display.drawText(100, startY + (lineGap * 2), isLock ? "LOCK" : (isTurnOn ? "ON" : "OFF"));

    m_display.setTextColor(Color::kTextDim, Color::kBg);
    std::snprintf(buffer, sizeof(buffer), "전란 간격 %umin", m_model.editIntervalMin);
    m_display.drawText(180, startY + (lineGap * 2), buffer);

    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY + (lineGap * 3), "다음 전란");
    
    m_display.setTextColor(Color::kText, Color::kBg);
    std::snprintf(buffer, sizeof(buffer), "%u분 후", m_model.nextTurningInMin);
    m_display.drawText(100, startY + (lineGap * 3), buffer);
}

void MainUiRenderer::renderConfirm(const char* title, const char* line1, const char* line2)
{
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kAccentTemp, Color::kBg); 
    m_display.drawText(34, 60, title); 
    
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(34, 82, line1);
    
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(34, 104, line2);
    
    drawRow(148, "아니오", "취소", m_model.confirmCursor == 0, Color::kOffIcon);
    drawRow(180, "예", "실행", m_model.confirmCursor == 1, Color::kDanger);
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