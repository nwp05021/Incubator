#include "ui/pages/PageAwsTest.h"
#include "ui/UiColors.h"
#include <cstdio>
#include <cstring>

namespace incubator::ui::pages
{
    void PageAwsTest::render(uint32_t)
    {
        char buffer[48];
        const AwsPublishEndpoint& endpoint = kAwsPublishEndpoints[m_model.awsEndpointCursor];

        drawRow(42, "Endpoint", endpoint.name, true, Color::kAccentTemp);
        drawRow(70, "MQTT", m_model.cloudConnected ? "READY" : "OFF", false,
                m_model.cloudConnected ? Color::kOnIcon : Color::kWarn);

        std::snprintf(buffer, sizeof(buffer), "%.1fC %.0f%%",
                      static_cast<double>(m_model.displayTempC),
                      static_cast<double>(m_model.displayHumidPct));
        drawRow(98, "Sensor", buffer, false, Color::kText);

        if (m_model.awsTestRunning) {
            std::snprintf(buffer, sizeof(buffer), "SENDING");
        } else if (m_model.awsTestCount == 0U) {
            std::snprintf(buffer, sizeof(buffer), "READY");
        } else {
            std::snprintf(buffer, sizeof(buffer), "%s #%u",
                          m_model.awsLastResult ? "OK" : "FAIL",
                          static_cast<unsigned>(m_model.awsTestCount));
        }
        drawRow(126, "Result", buffer, false,
                m_model.awsLastResult ? Color::kOnIcon : Color::kTextDim);

        m_display.setTextSize(1);
        m_display.drawLine(16, 157, 304, 157, Color::kPanelSoft);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, 164, endpoint.topicFormat);

        for (uint8_t i = 0; i < 2; ++i) {
            uint8_t logIndex = static_cast<uint8_t>(i + 2U);
            if (m_model.awsTestLog[logIndex][0] == '\0') continue;

            uint32_t logColor = Color::kText;
            if (std::strstr(m_model.awsTestLog[logIndex], "OK")) logColor = Color::kOnIcon;
            if (std::strstr(m_model.awsTestLog[logIndex], "FAIL")) logColor = Color::kDanger;

            m_display.setTextColor(logColor, Color::kBg);
            m_display.drawText(16, 184 + static_cast<int>(i) * 18, m_model.awsTestLog[logIndex]);
        }
    }
}
