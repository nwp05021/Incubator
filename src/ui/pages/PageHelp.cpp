#include "ui/pages/PageHelp.h"
#include "ui/UiColors.h"

namespace incubator::ui::pages
{
    void PageHelp::render(uint32_t nowMs)
    {
        // Layout::kBodyY 위치 기준 (약 50 부근)
        const int startY = 44; 
        const int lineGap = 26;

        m_display.setTextSize(1);
        
        // 1. 타이틀 및 AWS 연결 상태
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(16, startY, "AWS IoT 통신 테스트");

        bool isConn = m_model.cloudConnected;
        m_display.setTextColor(isConn ? Color::kOnIcon : Color::kWarn, Color::kBg);
        m_display.drawText(16, startY + lineGap, isConn ? "MQTT: 연결됨 (Ready)" : "MQTT: 대기중 (Disconnected)");

        // 2. [테스트] 버튼 렌더링 (동적 피드백)
        uint32_t btnBg = m_model.awsTestRunning ? Color::kTextDim : Color::kPanel;
        uint32_t btnFg = m_model.awsTestRunning ? Color::kBg : Color::kText;
        
        // m_model.activePage == 2 일 때, 버튼 UI 제공
        drawButton(200, startY + lineGap - 6, 100, 28, m_model.awsTestRunning ? "전송 중.." : "테스트 실행", btnBg);
        // (drawButton 내부에서 색상과 텍스트를 모두 처리합니다)

        // 3. 로그 출력 콘솔 영역
        m_display.drawLine(16, startY + lineGap * 2, 304, startY + lineGap * 2, Color::kPanelSoft);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, startY + lineGap * 2 + 8, "[ 시스템 로그 ]");

        // 4개의 로그 라인 순차 렌더링
        for (uint8_t i = 0; i < 4; ++i) {
            if (m_model.awsTestLog[i][0] != '\0') {
                // 성공/실패 키워드에 따른 동적 컬러 하이라이팅
                uint32_t logColor = Color::kText;
                if (std::strstr(m_model.awsTestLog[i], "성공") || std::strstr(m_model.awsTestLog[i], "OK")) logColor = Color::kOnIcon;
                if (std::strstr(m_model.awsTestLog[i], "실패") || std::strstr(m_model.awsTestLog[i], "Error")) logColor = Color::kDanger;
                
                m_display.setTextColor(logColor, Color::kBg);
                m_display.drawText(16, startY + lineGap * 2 + 28 + (i * 20), m_model.awsTestLog[i]);
            }
        }
    }
}