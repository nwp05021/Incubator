#pragma once
#include <cstdint>

namespace incubator::ui::Color
{
    static constexpr uint32_t kBg         = 0x0000U;
    static constexpr uint32_t kHeader     = 0x1082U;
    static constexpr uint32_t kFooter     = 0x1082U;
    static constexpr uint32_t kText       = 0xFFFFU;
    static constexpr uint32_t kTextDim    = 0x7BEFU;
    static constexpr uint32_t kAccentTemp = 0xFD20U;
    static constexpr uint32_t kAccentHumi = 0x07FFU;
    static constexpr uint32_t kAlarmHigh  = 0xF800U;
    static constexpr uint32_t kAlarmLow   = 0x001FU;
    static constexpr uint32_t kOnIcon     = 0x07E0U;
    static constexpr uint32_t kOffIcon    = 0x4208U;
    static constexpr uint32_t kLockdown   = 0xFFE0U;
    static constexpr uint32_t kSelected   = 0x3166U;
    static constexpr uint32_t kDivider    = 0x4208U;
    static constexpr uint32_t kProgress   = 0x07E0U;
}
