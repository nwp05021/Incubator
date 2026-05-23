#include "ui/pages/PagePlanList.h"
#include "ui/UiColors.h"

namespace incubator::ui::pages
{
    void PagePlanList::render(uint32_t nowMs)
    {
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(12, 52, "Day   Temp    Humi    Turn    Int");
        
        for (uint8_t i = 0; i < m_model.planListCount; ++i) {
            const auto& item = m_model.planList[i];
            uint16_t day = item.day;
            int y = 68 + i * 28;
            
            bool sel = (day == m_model.editDay);
            
            uint32_t bg     = sel ? Color::kText : Color::kPanel;
            uint32_t border = sel ? Color::kText : Color::kPanelSoft;
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
}