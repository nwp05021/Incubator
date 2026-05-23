#include "ui/pages/PagePlanEdit.h"
#include "ui/UiColors.h"

namespace incubator::ui::pages
{
    void PagePlanEdit::render(uint32_t nowMs)
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

        uint32_t saveBg = (m_model.fieldCursor == 4U) ? Color::kText : Color::kPanel;
        uint32_t saveFg = (m_model.fieldCursor == 4U) ? Color::kBg   : Color::kText; 
        m_display.setTextColor(saveFg, saveBg);
        drawButton(20, 180, 130, 28, "저 장", saveBg);

        uint32_t cancelBg = (m_model.fieldCursor == 5U) ? Color::kText : Color::kPanel;
        uint32_t cancelFg = (m_model.fieldCursor == 5U) ? Color::kBg   : Color::kText; 
        m_display.setTextColor(cancelFg, cancelBg);
        drawButton(170, 180, 130, 28, "취 소", cancelBg);
   }
}