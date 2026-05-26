#include "ui/pages/PageAwsTest.h"
#include "ui/UiColors.h"
#include "config/AppConfig.h"   
#include <cstdio>
#include <cstring>
#include <esp_log.h>

namespace incubator::ui::pages
{
    void PageAwsTest::render(uint32_t)
    {
        char buffer[128];
        m_display.setTextSize(1);

        // =========================================================================
        // 1. AWS 연결 인프라 정보
        // =========================================================================
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, 25, "■ AWS IoT Endpoint:");
        
        m_display.setTextColor(Color::kAccentTemp, Color::kBg);
#ifdef AWS_IOT_ENDPOINT
        m_display.drawText(135, 25, AWS_IOT_ENDPOINT);
#else
        m_display.drawText(135, 25, "NOT DEFINED");
#endif

        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, 50, "■ Device ID:");
#ifdef INCUBATOR_DEVICE_ID
        std::snprintf(buffer, sizeof(buffer), "%s", INCUBATOR_DEVICE_ID);
#else
        std::snprintf(buffer, sizeof(buffer), "UNKNOWN-ID");
#endif
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(120, 50, buffer);

        // =========================================================================
        // 2. 실시간 MQTT 연결 상태 
        // =========================================================================
        m_display.drawLine(16, 70, 304, 70, Color::kPanelSoft);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, 78, "■ MQTT Connection Status:");

        if (m_model.cloudConnected) {
            m_display.setTextColor(Color::kAccentHumi, Color::kBg); 
            m_display.drawText(214, 78, "[ ONLINE ]");
        } else {
            m_display.setTextColor(Color::kWarn, Color::kBg);       
            m_display.drawText(214, 78, "[ OFFLINE ]");
        }

        // =========================================================================
        // 3. 수동 패킷 전송 테스트 로그 모니터링 (UiModel 전역 변수로 교체 매핑)
        // =========================================================================
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, 105, "■ Test Tx Count:");
        // 💡 m_txCount 대신 m_model.awsTestCount 바인딩
        std::snprintf(buffer, sizeof(buffer), "%lu times", static_cast<unsigned long>(m_model.awsTestCount));        
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(144, 105, buffer);

        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, 125, "■ Last Tx Result:");
        // 💡 m_model.awsTestCount와 m_model.awsLastResult 기준 판정으로 전면 교체
        if (m_model.awsTestCount == 0) {
            m_display.setTextColor(Color::kTextDim, Color::kBg);
            m_display.drawText(146, 125, "No history");
        } else if (m_model.awsLastResult) {
            m_display.setTextColor(Color::kAccentHumi, Color::kBg); 
            m_display.drawText(146, 125, "SUCCESS (Check DB)");
        } else {
            m_display.setTextColor(Color::kWarn, Color::kBg);
            m_display.drawText(146, 125, "FAILED");
        }

        m_display.drawLine(16, 150, 304, 150, Color::kPanelSoft);

        // =========================================================================
        // 4. 개편된 [전송] [취소] 대화형 버튼 UI 렌더링
        // =========================================================================
        m_display.setTextSize(1);
        
        if (m_model.confirmCursor == 0) {
            m_display.setTextColor(Color::kBg, Color::kAccentTemp);
            m_display.drawText(60, 185, " [ 전  송 ] ");
        } else {
            m_display.setTextColor(Color::kText, Color::kPanelSoft);
            m_display.drawText(60, 185, "   전  송   ");
        }

        if (m_model.confirmCursor == 1) {
            m_display.setTextColor(Color::kBg, Color::kWarn);
            m_display.drawText(180, 185, " [ 취  소 ] ");
        } else {
            m_display.setTextColor(Color::kText, Color::kPanelSoft);
            m_display.drawText(180, 185, "   취  소   ");
        }
    }
}