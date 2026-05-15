#pragma once
#include "devices/Aht20Driver.h"
#include "domain/RuntimeState.h"
#include <cstdint>

namespace incubator::modules
{
    class SensorManager
    {
    public:
        static constexpr uint32_t kPollIntervalMs = 2000U;
        static constexpr uint32_t kMeasureDelayMs = 80U;

        SensorManager(devices::Aht20Driver& driver,
                      domain::RuntimeState& state)
            : m_driver(driver), m_state(state) {}

        void tick(uint32_t nowMs);
        bool isHealthy() const { return m_driver.isConnected(); }

    private:
        devices::Aht20Driver& m_driver;
        domain::RuntimeState& m_state;

        enum class Phase { Idle, WaitResult };
        Phase    m_phase = Phase::Idle;
        uint32_t m_lastPollMs = 0;
        uint32_t m_triggerMs = 0;
        uint32_t m_lastGoodMs = 0;
        uint8_t  m_failCount = 0;

        void markReadFailed(uint32_t nowMs);
    };
}
