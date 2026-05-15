#pragma once
#include "domain/RuntimeState.h"
#include "domain/AppSettings.h"
#include "devices/GpioOutput.h"
#include <cstdint>

namespace incubator::modules
{
    class ClimateModule
    {
    public:
        static constexpr uint32_t kTickIntervalMs = 500U;
        static constexpr uint32_t kRelayMinSwitchMs = 10000U;

        ClimateModule(domain::RuntimeState&      state,
                      const domain::AppSettings& settings,
                      devices::GpioOutput&       heater,
                      devices::GpioOutput&       humidifier,
                      devices::GpioOutput&       buzzer)
            : m_state(state), m_settings(settings),
              m_heater(heater), m_humidifier(humidifier), m_buzzer(buzzer) {}

        void tick(uint32_t nowMs);

    private:
        domain::RuntimeState&      m_state;
        const domain::AppSettings& m_settings;
        devices::GpioOutput&       m_heater;
        devices::GpioOutput&       m_humidifier;
        devices::GpioOutput&       m_buzzer;

        uint32_t m_lastMs = 0;
        uint32_t m_tempAlarmMs = 0;
        uint32_t m_humiAlarmMs = 0;
        uint32_t m_lastHeaterSwitchMs = 0;
        uint32_t m_lastHumidifierSwitchMs = 0;

        void controlTemp(uint32_t delta);
        void controlHumidity(uint32_t delta);
        void checkAlarms(uint32_t delta);
        void allOff();
        bool canSwitch(uint32_t nowMs, uint32_t& lastSwitchMs) const;
    };
}
