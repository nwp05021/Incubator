#include "modules/IncubationScheduler.h"
#include "policy/DayResolver.h"
#include "domain/IncubationPlanRow.h"
#include <ctime>
#include <esp_log.h>

static const char* TAG = "Scheduler";

namespace incubator::modules
{

void IncubationScheduler::tick(uint32_t nowMs)
{
    if (!m_batch.active) {
        m_state.batchActive = false;
        return;
    }
    if (nowMs - m_lastMs < kTickIntervalMs) return;

    uint32_t nowEpoch = static_cast<uint32_t>(time(nullptr));
    uint16_t day = policy::DayResolver::resolve(m_batch.startEpoch, nowEpoch, m_batch.totalDays);

    m_state.currentDay = day;
    m_state.totalDays = m_batch.totalDays;
    m_state.batchActive = true;
    m_state.lockdownActive = (day >= m_batch.lockdownStartDay);
    m_state.lockdownStartDay = m_batch.lockdownStartDay;

    const domain::IncubationPlanRow* row = m_plan.getRow(day);
    if (!row) {
        ESP_LOGE(TAG, "Plan row missing for day %u — entering safe mode", day);
        m_state.safeMode = true;
        return;
    }

    applyRow(*row);
    m_lastMs = nowMs;
}

void IncubationScheduler::applyRow(const domain::IncubationPlanRow& row)
{
    m_state.targetTempC = row.targetTempC;
    m_state.targetHumidityPct = row.targetHumidityPct;
    m_state.turningEnabled = row.turningEnabled && !m_state.lockdownActive;
    m_state.nextTurningInMin = 0;
    m_state.lockdownActive = (m_state.currentDay >= m_batch.lockdownStartDay);
    m_state.turnerOn = m_state.turningEnabled && m_state.turnerOn;
}

} // namespace incubator::modules
