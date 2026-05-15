#pragma once
#include "domain/RuntimeState.h"
#include "domain/AppSettings.h"
#include "domain/IncubationPlanTable.h"
#include "devices/GpioOutput.h"
#include <cstdint>

namespace incubator::modules
{
    class TurningModule
    {
    public:
        static constexpr uint32_t kTickIntervalMs = 1000U;

        TurningModule(domain::RuntimeState&            state,
                      const domain::AppSettings&       settings,
                      const domain::IncubationPlanTable& plan,
                      devices::GpioOutput&             turner)
            : m_state(state), m_settings(settings), m_plan(plan), m_turner(turner) {}

        void tick(uint32_t nowMs);

    private:
        domain::RuntimeState&            m_state;
        const domain::AppSettings&       m_settings;
        const domain::IncubationPlanTable& m_plan;
        devices::GpioOutput&             m_turner;

        uint32_t m_lastMs = 0;
        uint32_t m_turningOnMs = 0;
        bool     m_isTurning = false;
    };
}
