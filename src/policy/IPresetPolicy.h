#pragma once
#include "domain/IncubationPlanTable.h"
#include "domain/IncubationBatch.h"

namespace incubator::policy
{
    class IPresetPolicy
    {
    public:
        virtual ~IPresetPolicy() = default;
        virtual void fill(domain::IncubationPlanTable& table,
                          domain::IncubationBatch&     batch) const = 0;
    };
}
