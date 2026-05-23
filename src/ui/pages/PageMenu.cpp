#include "ui/pages/PageMenu.h"

namespace incubator::ui::pages
{
    void PageMenu::render(uint32_t nowMs)
    {
        constexpr uint8_t kMaxVisible = 5;
        uint8_t topIndex = 0;
        
        if (m_model.menuCursor >= kMaxVisible) {
            topIndex = m_model.menuCursor - kMaxVisible + 1;
        }

        for (uint8_t v = 0; v < kMaxVisible; ++v) {
            uint8_t i = topIndex + v;
            if (i >= kMainMenuCount) break; // 🔥 통합된 메뉴 갯수 상수를 사용

            int y = 54 + static_cast<int>(v) * 31; 
            bool selected = (m_model.menuCursor == i);

            uint32_t bg     = selected ? Color::kText : 0x2146U; 
            uint32_t border = selected ? Color::kText : 0x31CDU;
            uint32_t textFg = selected ? Color::kBg   : Color::kText; 
            uint32_t descFg = selected ? Color::kBg   : Color::kTextDim; 

            m_display.fillRect(12, y, 296, 27, bg);
            m_display.drawRect(12, y, 296, 27, border);

            m_display.setTextSize(1);
            m_display.setTextColor(textFg, bg);
            // 🔥 통합 배열에서 이름(name)을 가져옵니다.
            m_display.drawText(24, y + 7, kMainMenuItems[i].name);

            m_display.setTextColor(descFg, bg);
            // 🔥 통합 배열에서 설명(desc)을 가져옵니다.
            m_display.drawText(196, y + 7, kMainMenuItems[i].desc);
        }
    }
}