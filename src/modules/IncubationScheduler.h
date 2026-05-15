#pragma once
#include "domain/RuntimeState.h"
#include "domain/IncubationBatch.h"
#include "domain/IncubationPlanTable.h"
#include <cstdint>

namespace incubator::modules
{
    class IncubationScheduler
    {
    public:
        static constexpr uint32_t kTickIntervalMs = 10000U;

        IncubationScheduler(domain::RuntimeState&              state,
                            const domain::IncubationBatch&     batch,
                            const domain::IncubationPlanTable& plan)
            : m_state(state), m_batch(batch), m_plan(plan) {}

        void tick(uint32_t nowMs);

    private:
        domain::RuntimeState&              m_state;
        const domain::IncubationBatch&     m_batch;
        const domain::IncubationPlanTable& m_plan;

        uint32_t m_lastMs = 0;

        void applyRow(const domain::IncubationPlanRow& row);
    };
}
