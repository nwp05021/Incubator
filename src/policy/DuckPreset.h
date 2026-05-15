#pragma once
#include "IPresetPolicy.h"

namespace incubator::policy
{
    class DuckPreset final : public IPresetPolicy
    {
    public:
        void fill(domain::IncubationPlanTable& table,
                  domain::IncubationBatch&     batch) const override;
    };
}
