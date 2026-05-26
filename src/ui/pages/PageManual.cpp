#include "ui/pages/PageManual.h"

namespace incubator::ui::pages
{
    void PageManual::render(uint32_t nowMs)
    {
        drawRow(62, "히터 제어", m_model.heaterOn ? "ON" : "OFF", m_model.manualCursor == 0,
                m_model.heaterOn ? Color::kOnIcon : Color::kOffIcon, 1);
        drawRow(96, "가습 제어", m_model.humidifierOn ? "ON" : "OFF", m_model.manualCursor == 1,
                m_model.humidifierOn ? Color::kOnIcon : Color::kOffIcon, 1);
        drawRow(130, "전란 모터", m_model.turnerOn ? "ON" : "OFF", m_model.manualCursor == 2,
                m_model.turnerOn ? Color::kOnIcon : Color::kOffIcon, 1);
        drawRow(164, "순환 팬", m_model.fanOn ? "ON" : "OFF", m_model.manualCursor == 3,
                m_model.fanOn ? Color::kOnIcon : Color::kOffIcon, 1);
   }
}