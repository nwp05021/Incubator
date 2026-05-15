#include "modules/ClimateModule.h"
#include <esp_log.h>

namespace incubator::modules
{

void ClimateModule::tick(uint32_t nowMs)
{
    uint32_t delta = nowMs - m_lastMs;
    if (delta < kTickIntervalMs) return;
    m_lastMs = nowMs;

    if (m_state.manualMode) {
        return;
    }

    if (m_state.safeMode || !m_state.batchActive) {
        allOff();
        return;
    }

    controlTemp(nowMs);
    controlHumidity(nowMs);
    checkAlarms(delta);
}

void ClimateModule::controlTemp(uint32_t nowMs)
{
    if (!m_state.tempSensorOk) {
        if (m_heater.isOn() && canSwitch(nowMs, m_lastHeaterSwitchMs)) {
            m_heater.off();
        }
        m_state.heaterOn = m_heater.isOn();
        return;
    }

    float cur = m_state.currentTempC;
    float target = m_state.targetTempC;
    float hyst = m_settings.tempHysteresis;

    if (cur < target - hyst && !m_heater.isOn() && canSwitch(nowMs, m_lastHeaterSwitchMs)) {
        m_heater.on();
    } else if (cur > target + hyst && m_heater.isOn() && canSwitch(nowMs, m_lastHeaterSwitchMs)) {
        m_heater.off();
    }
    m_state.heaterOn = m_heater.isOn();
}

void ClimateModule::controlHumidity(uint32_t nowMs)
{
    if (!m_state.humiSensorOk) {
        if (m_humidifier.isOn() && canSwitch(nowMs, m_lastHumidifierSwitchMs)) {
            m_humidifier.off();
        }
        m_state.humidifierOn = m_humidifier.isOn();
        return;
    }

    float cur = m_state.currentHumidityPct;
    float target = m_state.targetHumidityPct;
    float hyst = m_settings.humidityHysteresis;

    if (cur < target - hyst && !m_humidifier.isOn() && canSwitch(nowMs, m_lastHumidifierSwitchMs)) {
        m_humidifier.on();
    } else if (cur > target + hyst && m_humidifier.isOn() && canSwitch(nowMs, m_lastHumidifierSwitchMs)) {
        m_humidifier.off();
    }
    m_state.humidifierOn = m_humidifier.isOn();
}

void ClimateModule::checkAlarms(uint32_t delta)
{
    float tempErr = m_state.currentTempC - m_state.targetTempC;
    bool tempOob = (tempErr > m_settings.tempAlarmHighOffsetC) ||
                   (tempErr < -m_settings.tempAlarmLowOffsetC);

    if (tempOob && m_state.tempSensorOk) {
        m_tempAlarmMs += delta;
        if (m_tempAlarmMs >= m_settings.alarmConfirmMs) {
            m_state.tempAlarmActive = true;
            if (m_settings.alarmEnabled) {
                m_buzzer.on();
            }
        }
    } else {
        m_tempAlarmMs = 0;
        m_state.tempAlarmActive = false;
    }

    float humiErr = m_state.currentHumidityPct - m_state.targetHumidityPct;
    bool humiOob = (humiErr > m_settings.humidAlarmHighOffsetPct) ||
                   (humiErr < -m_settings.humidAlarmLowOffsetPct);

    if (humiOob && m_state.humiSensorOk) {
        m_humiAlarmMs += delta;
        if (m_humiAlarmMs >= m_settings.alarmConfirmMs) {
            m_state.humiAlarmActive = true;
            if (m_settings.alarmEnabled) {
                m_buzzer.on();
            }
        }
    } else {
        m_humiAlarmMs = 0;
        m_state.humiAlarmActive = false;
    }

    if (!m_state.tempAlarmActive && !m_state.humiAlarmActive) {
        m_buzzer.off();
    }
}

void ClimateModule::allOff()
{
    m_heater.off();
    m_humidifier.off();
    m_buzzer.off();
    m_state.heaterOn = false;
    m_state.humidifierOn = false;
    m_state.tempAlarmActive = false;
    m_state.humiAlarmActive = false;
}

bool ClimateModule::canSwitch(uint32_t nowMs, uint32_t& lastSwitchMs) const
{
    if (lastSwitchMs == 0U || nowMs - lastSwitchMs >= kRelayMinSwitchMs) {
        lastSwitchMs = nowMs;
        return true;
    }
    return false;
}

} // namespace incubator::modules
