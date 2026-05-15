#include "policy/DuckPreset.h"
#include "domain/IncubationPlanRow.h"
#include <ctime>

namespace incubator::policy
{

void DuckPreset::fill(domain::IncubationPlanTable& table,
                      domain::IncubationBatch&     batch) const
{
    batch.totalDays = 28;
    batch.lockdownStartDay = 25;
    table.clear();

    for (uint16_t d = 1; d <= 24; ++d) {
        table.rows[table.rowCount++] =
            domain::IncubationPlanRow::make(d, 37.5f, 55.0f, true, 180U, true);
    }
    for (uint16_t d = 25; d <= 28; ++d) {
        table.rows[table.rowCount++] =
            domain::IncubationPlanRow::make(d, 37.0f, 65.0f, false, 0U, true);
    }

    table.tableVersion = 1;
    table.lastUpdatedAt = static_cast<uint32_t>(time(nullptr));
}

} // namespace incubator::policy
