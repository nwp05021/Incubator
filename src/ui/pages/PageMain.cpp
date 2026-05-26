#include "ui/pages/PageMain.h"
#include "ui/UiColors.h"

namespace incubator::ui::pages
{
    void PageMain::render(uint32_t nowMs)
    {
        char buffer[48];

        m_display.drawLine(160, 36, 160, 154, Color::kDivider);
        m_display.drawLine(20, 146, 140, 146, Color::kPanelSoft);
        m_display.drawLine(180, 146, 300, 146, Color::kPanelSoft);

        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(22, 42, "온도"); 

        if (m_model.tempSensorFault) {
            m_display.setTextSize(1);
            m_display.setTextColor(Color::kDanger, Color::kBg);
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
            m_display.setTextColor(Color::kDanger, Color::kBg);
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
        std::snprintf(buffer, sizeof(buffer), "목표: %.1f C", m_model.targetTempC);
        drawPill(20, 172, 120, buffer, Color::kAccentTemp); 

        std::snprintf(buffer, sizeof(buffer), "목표: %.0f %%", m_model.targetHumidPct);
        drawPill(180, 172, 120, buffer, Color::kAccentHumi);
    }
}