#pragma once
#include "IncubationSpecies.h"
#include <cstdint>
#include <cstring>

namespace incubator::domain
{
    struct IncubationBatch
    {
        Species  species = Species::Chicken;
        uint16_t totalDays = 0;
        uint16_t lockdownStartDay = 0;
        bool     active = false;
        uint32_t startEpoch = 0; // Unix timestamp seconds
        char     batchId[16] = {};

        bool isValid() const
        {
            return totalDays > 0 && lockdownStartDay > 0 && batchId[0] != '\0';
        }
    };
}
