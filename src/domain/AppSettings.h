#pragma once
#include <cstdint>

namespace incubator::domain
{
    struct AppSettings
    {
        float    tempHysteresis           = 0.1f;
        float    humidityHysteresis       = 2.0f;
        float    tempAlarmHighOffsetC     = 2.0f;
        float    tempAlarmLowOffsetC      = 2.0f;
        float    humidAlarmHighOffsetPct  = 10.0f;
        float    humidAlarmLowOffsetPct   = 10.0f;
        uint32_t alarmConfirmMs           = 60000U;
        bool     alarmEnabled             = true;
        uint16_t turningDurationMin       = 3;
        char     wifiSsid[64]             = {};
        char     wifiPassword[64]         = {};
        bool     wifiConfigured           = false;
        uint32_t bootProvisionTimeoutMs   = 120000U;
        uint32_t menuProvisionTimeoutMs   = 90000U;

        static AppSettings defaults()
        {
            return AppSettings();
        }

        bool isValid() const
        {
            return tempHysteresis >= 0.1f && tempHysteresis <= 5.0f
                && humidityHysteresis >= 0.5f && humidityHysteresis <= 20.0f
                && tempAlarmHighOffsetC >= 0.5f && tempAlarmLowOffsetC >= 0.5f
                && humidAlarmHighOffsetPct >= 1.0f && humidAlarmLowOffsetPct >= 1.0f
                && alarmConfirmMs >= 1000U
                && turningDurationMin > 0
                && bootProvisionTimeoutMs >= 10000U
                && menuProvisionTimeoutMs >= 10000U;
        }
    };
}
