#pragma once
#include <cstdint>
#include <cstring>

namespace incubator::domain
{
    struct RuntimeState
    {
        float    currentTempC         = 0.0f;
        float    currentHumidityPct   = 0.0f;
        float    targetTempC          = 37.5f;
        float    targetHumidityPct    = 55.0f;
        bool     tempSensorOk         = false;
        bool     humiSensorOk         = false;
        bool     tempSensorWarning    = false;
        bool     humiSensorWarning    = false;
        bool     heaterOn             = false;
        bool     humidifierOn         = false;
        bool     fanOn                = false;
        bool     turnerOn             = false;
        bool     tempAlarmActive      = false;
        bool     humiAlarmActive      = false;
        bool     safeMode             = false;
        bool     batchActive          = false;
        bool     lockdownActive       = false;
        bool     turningEnabled       = true;
        bool     cloudConnected       = false;
        bool     manualMode           = false;
        bool     wifiConnected        = false;
        uint16_t currentDay          = 1;
        uint16_t totalDays           = 21;
        uint16_t nextTurningInMin    = 0;
        uint16_t lockdownStartDay    = 0;
        uint32_t uptimeMs            = 0;
        uint32_t bootCount           = 0;
        uint32_t lastTurningMs       = 0;
        uint32_t batchStartEpoch     = 0;
        char     ipAddress[16]       = {};

        static RuntimeState zero()
        {
            RuntimeState state{};
            return state;
        }
    };
}
