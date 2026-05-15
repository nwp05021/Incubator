#include "policy/PlanGenerator.h"
#include "policy/ChickenPreset.h"
#include "policy/DuckPreset.h"
#include "policy/QuailPreset.h"
#include "policy/GoosePreset.h"

namespace incubator::policy
{

bool PlanGenerator::generate(domain::Species species,
                             domain::IncubationBatch& batch,
                             domain::IncubationPlanTable& table)
{
    table.clear();
    IPresetPolicy const* preset = nullptr;
    ChickenPreset chicken;
    DuckPreset duck;
    QuailPreset quail;
    GoosePreset goose;

    switch (species) {
        case domain::Species::Chicken: preset = &chicken; break;
        case domain::Species::Duck:    preset = &duck;    break;
        case domain::Species::Quail:   preset = &quail;   break;
        case domain::Species::Goose:   preset = &goose;   break;
        case domain::Species::Custom:  preset = &chicken; break;
        default:                       preset = &chicken; break;
    }

    preset->fill(table, batch);
    return table.isValid();
}

} // namespace incubator::policy
