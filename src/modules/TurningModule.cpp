#include "modules/TurningModule.h"
#include <esp_log.h>

namespace incubator::modules
{

    uint32_t savedMillis = 0;

    void TurningModule::tick(uint32_t nowMs)
    {
        if (nowMs - m_lastMs < kTickIntervalMs) return;
        m_lastMs = nowMs;

        if (m_state.manualMode) {
            return;
        }

        if (!m_state.batchActive || m_state.lockdownActive || !m_state.turningEnabled || m_state.safeMode) {
            m_turner.off();
            m_isTurning = false;
            m_state.turnerOn = false;
            m_state.nextTurningInMin = 0;
            return;
        }

        const auto* planRow = m_plan.getRow(m_state.currentDay);
        uint32_t intervalMin = planRow ? planRow->turningIntervalMin : 0;
        uint32_t intervalMs = static_cast<uint32_t>(intervalMin) * 60000UL;
        uint32_t durationMs = static_cast<uint32_t>(m_settings.turningDurationMin) * 60000UL;

        if (!m_isTurning) {
            uint32_t elapsed = nowMs - m_state.lastTurningMs;
            if (intervalMs > elapsed) {
                m_state.nextTurningInMin = (intervalMs - elapsed) / 60000U;
            } else {
                m_state.nextTurningInMin = 0;
            }

            if (elapsed >= intervalMs) {
                m_turner.on();
                m_isTurning = true;
                m_turningOnMs = nowMs;
                m_state.turnerOn = true;
                m_state.lastTurningMs = nowMs;
                m_state.nextTurningInMin = 0;
                ESP_LOGI("Turning", "Turner ON for %u min", m_settings.turningDurationMin);
            }
        } else {
            if (nowMs - m_turningOnMs >= durationMs) {
                m_turner.off();
                m_isTurning = false;
                m_state.turnerOn = false;
                ESP_LOGI("Turning", "Turner OFF");
            }
        }
    }

} // namespace incubator::modules
