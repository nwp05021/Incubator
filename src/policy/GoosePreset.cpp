#include "policy/GoosePreset.h"
#include "domain/IncubationPlanRow.h"
#include <ctime>

namespace incubator::policy
{

void GoosePreset::fill(domain::IncubationPlanTable& table,
                       domain::IncubationBatch&     batch) const
{
    batch.totalDays = 30;
    batch.lockdownStartDay = 27;
    table.clear();

    for (uint16_t d = 1; d <= 26; ++d) {
        table.rows[table.rowCount++] =
            domain::IncubationPlanRow::make(d, 37.4f, 55.0f, true, 180U, true);
    }
    for (uint16_t d = 27; d <= 30; ++d) {
        table.rows[table.rowCount++] =
            domain::IncubationPlanRow::make(d, 37.0f, 65.0f, false, 0U, true);
    }

    table.tableVersion = 1;
    table.lastUpdatedAt = static_cast<uint32_t>(time(nullptr));
}

} // namespace incubator::policy
