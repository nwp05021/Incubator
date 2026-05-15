#pragma once
#include "domain/IncubationPlanTable.h"
#include "domain/IncubationBatch.h"
#include "domain/IncubationSpecies.h"

namespace incubator::policy
{
    class PlanGenerator
    {
    public:
        static bool generate(domain::Species              species,
                             domain::IncubationBatch&     batch,
                             domain::IncubationPlanTable& table);
    };
}
