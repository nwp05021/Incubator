#include "ui/pages/PageStartDate.h"
#include "ui/UiColors.h"

namespace incubator::ui::pages
{
    void PageStartDate::render(uint32_t nowMs)
    {
        char dateStr[8];
        static constexpr const char* labels[] = { "년", "월", "일" };
        int values[] = { m_model.editBatchYear, m_model.editBatchMonth, m_model.editBatchDay };

        for (int i = 0; i < 3; ++i) {
            int x = 16 + i * 98;
            int y = 60; 
            
            bool fieldSel = (m_model.screen == UiScreen::StartDate && m_model.fieldCursor == i);
            
            uint32_t valBg = fieldSel ? Color::kText : Color::kPanel;
            uint32_t valFg = fieldSel ? Color::kBg   : Color::kText; 
            uint32_t boxBorder = fieldSel ? Color::kText : Color::kPanelSoft;

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

        const uint32_t saveBg = (m_model.fieldCursor == 3U) ? Color::kText : Color::kPanel;
        const uint32_t saveFg = (m_model.fieldCursor == 3U) ? Color::kBg   : Color::kText; 

        m_display.setTextSize(1);
        m_display.setTextColor(saveFg, saveBg);
        drawButton(20, 164, 130, 32, "등 록", saveBg); 

        const uint32_t cancelBg = (m_model.fieldCursor == 4U) ? Color::kText : Color::kPanel;
        const uint32_t cancelFg = (m_model.fieldCursor == 4U) ? Color::kBg   : Color::kText; 

        m_display.setTextColor(cancelFg, cancelBg);
        drawButton(170, 164, 130, 32, "취 소", cancelBg);
    }
}