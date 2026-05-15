#include "policy/QuailPreset.h"
#include "domain/IncubationPlanRow.h"
#include <ctime>

namespace incubator::policy
{

void QuailPreset::fill(domain::IncubationPlanTable& table,
                       domain::IncubationBatch&     batch) const
{
    batch.totalDays = 17;
    batch.lockdownStartDay = 15;
    table.clear();

    for (uint16_t d = 1; d <= 14; ++d) {
        table.rows[table.rowCount++] =
            domain::IncubationPlanRow::make(d, 37.8f, 45.0f, true, 120U, true);
    }
    for (uint16_t d = 15; d <= 17; ++d) {
        table.rows[table.rowCount++] =
            domain::IncubationPlanRow::make(d, 37.2f, 60.0f, false, 0U, true);
    }

    table.tableVersion = 1;
    table.lastUpdatedAt = static_cast<uint32_t>(time(nullptr));
}

} // namespace incubator::policy
