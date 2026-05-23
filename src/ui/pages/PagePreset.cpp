#include "ui/pages/PagePreset.h"
#include "ui/UiColors.h"

namespace incubator::ui::pages
{
    void PagePreset::render(uint32_t nowMs)
    {
        if (m_model.presetConfirm) {
            renderConfirm("프리셋 적용", "정말 다시 생성하겠습니까?", "현재 계획이 새 프리셋으로 바뀝니다.");
            return;
        }
        for (uint8_t i = 0; i < 4; ++i) {
            char no[8];
            std::snprintf(no, sizeof(no), "%u", i + 1U);
            drawRow(56 + i * 36, no, presetName(i), m_model.presetCursor == i, Color::kText, 1);
        }
    }
}