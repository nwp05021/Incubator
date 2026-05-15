#include "policy/ChickenPreset.h"
#include "domain/IncubationPlanRow.h"
#include <ctime>

namespace incubator::policy
{

void ChickenPreset::fill(domain::IncubationPlanTable& table,
                         domain::IncubationBatch&     batch) const
{
    batch.totalDays = 21;
    batch.lockdownStartDay = 19;
    table.clear();

    for (uint16_t d = 1; d <= 18; ++d) {
        table.rows[table.rowCount++] =
            domain::IncubationPlanRow::make(d, 37.8f, 55.0f, true, 120U, true);
    }
    for (uint16_t d = 19; d <= 21; ++d) {
        table.rows[table.rowCount++] =
            domain::IncubationPlanRow::make(d, 37.2f, 65.0f, false, 0U, true);
    }

    table.tableVersion = 1;
    table.lastUpdatedAt = static_cast<uint32_t>(time(nullptr));
}

} // namespace incubator::policy
