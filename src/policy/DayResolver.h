#pragma once
#include <cstdint>

namespace incubator::policy
{
    class DayResolver
    {
    public:
        static uint16_t resolve(uint32_t startEpoch,
                                uint32_t nowEpoch,
                                uint16_t totalDays)
        {
            if (totalDays == 0) return 0;
            if (nowEpoch <= startEpoch) return 1;
            uint32_t elapsed = nowEpoch - startEpoch;
            uint16_t day = static_cast<uint16_t>(elapsed / 86400U) + 1;
            return (day > totalDays) ? totalDays : day;
        }

        static uint32_t completionEpoch(uint32_t startEpoch,
                                        uint16_t totalDays)
        {
            return startEpoch + static_cast<uint32_t>(totalDays) * 86400U;
        }

        static uint8_t progressPct(uint16_t currentDay, uint16_t totalDays)
        {
            if (totalDays == 0) return 0;
            uint32_t pct = static_cast<uint32_t>(currentDay) * 100U / totalDays;
            return (pct > 100) ? 100 : static_cast<uint8_t>(pct);
        }
    };
}
