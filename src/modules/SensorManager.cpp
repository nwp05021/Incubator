#include "modules/SensorManager.h"
#include "devices/Aht20Driver.h"

namespace incubator::modules
{

void SensorManager::tick(uint32_t nowMs)
{
    if (m_phase == Phase::Idle) {
        if (nowMs - m_lastPollMs >= kPollIntervalMs) {
            if (m_driver.triggerMeasurement()) {
                m_triggerMs = nowMs;
                m_phase = Phase::WaitResult;
            } else {
                markReadFailed(nowMs);
                m_lastPollMs = nowMs;
            }
        }
        return;
    }

    if (m_phase == Phase::WaitResult) {
        if (nowMs - m_triggerMs >= kMeasureDelayMs) {
            bool ok = m_driver.fetchResult();
            if (ok && m_driver.isCacheValid()) {
                m_state.currentTempC = m_driver.getCachedTemp();
                m_state.currentHumidityPct = m_driver.getCachedHumi();
                m_state.tempSensorOk = true;
                m_state.humiSensorOk = true;
                m_state.tempSensorWarning = false;
                m_state.humiSensorWarning = false;
                m_lastGoodMs = nowMs;
                m_failCount = 0;
            } else {
                markReadFailed(nowMs);
            }
            m_state.uptimeMs = nowMs;
            m_lastPollMs = nowMs;
            m_phase = Phase::Idle;
        }
    }
}

void SensorManager::markReadFailed(uint32_t nowMs)
{
    if (m_failCount < 255U) ++m_failCount;

    m_state.tempSensorWarning = true;
    m_state.humiSensorWarning = true;

    // Keep controlling from the last valid sample. A transient AHT20/I2C miss
    // should notify the user, not latch the incubator into stopped relays.
    if (m_lastGoodMs != 0U) {
        return;
    }

    m_state.tempSensorOk = false;
    m_state.humiSensorOk = false;
}

} // namespace incubator::modules
