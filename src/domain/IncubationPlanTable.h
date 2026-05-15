#pragma once
#include "IncubationPlanRow.h"
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace incubator::domain
{
    struct IncubationPlanTable
    {
        static constexpr size_t kMaxRows = 64;

        uint32_t tableVersion = 0;
        uint32_t lastUpdatedAt = 0;
        uint16_t rowCount = 0;
        IncubationPlanRow rows[kMaxRows] = {};

        void clear()
        {
            tableVersion = 0;
            lastUpdatedAt = 0;
            rowCount = 0;
            for (auto& row : rows) {
                row = {}; // 또는 row = incubator::domain::IncubationPlanRow();
            }
        }

        const IncubationPlanRow* getRow(uint16_t day) const
        {
            if (day == 0 || day > rowCount) return nullptr;
            return &rows[day - 1];
        }

        IncubationPlanRow* getRowMutable(uint16_t day)
        {
            if (day == 0 || day > rowCount) return nullptr;
            return &rows[day - 1];
        }

        bool isValid() const
        {
            if (rowCount == 0 || rowCount > kMaxRows) return false;
            for (uint16_t i = 0; i < rowCount; ++i) {
                if (rows[i].day != i + 1) return false;
            }
            return true;
        }
    };
}
