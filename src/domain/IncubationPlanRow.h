#pragma once
#include <cstdint>

namespace incubator::domain
{
    struct IncubationPlanRow
    {
        uint16_t day = 0;
        float    targetTempC = 0.0f;
        float    targetHumidityPct = 0.0f;
        bool     turningEnabled = false;
        uint16_t turningIntervalMin = 0;
        bool     ventFanEnabled = false;
        bool     userOverridden = false;

        static IncubationPlanRow make(uint16_t d,
                                      float temp,
                                      float humi,
                                      bool turning,
                                      uint16_t interval,
                                      bool ventFan)
        {
            IncubationPlanRow row;
            row.day = d;
            row.targetTempC = temp;
            row.targetHumidityPct = humi;
            row.turningEnabled = turning;
            row.turningIntervalMin = interval;
            row.ventFanEnabled = ventFan;
            row.userOverridden = false;
            return row;
        }
    };
}
