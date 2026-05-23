#include "ui/pages/PageFactoryReset.h"
#include "ui/UiColors.h"

namespace incubator::ui::pages
{
    void PageFactoryReset::render(uint32_t nowMs)
    {
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kDanger, Color::kBg);
        m_display.drawText(24, 76, "모든 설정, WiFi, 부화 계획이 삭제됩니다.");
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(24, 100, "실행하려면 버튼을 10초간 계속 누르세요.");
        drawProgressBar(32, 138, 256, 18, m_model.factoryProgressPct, Color::kDanger);
        char pct[16];
        std::snprintf(pct, sizeof(pct), "%u%%", m_model.factoryProgressPct);
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(144, 166, pct);
    }
}