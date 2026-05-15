#include "app/AppController.h"
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <cstdio>
#include <cstring>

namespace incubator::app
{

bool AppController::applyCommand(Cmd cmd, const void* payload, size_t sz)
{
    switch (cmd)
    {
    case Cmd::StartBatch:
        if (!payload || sz != sizeof(domain::IncubationBatch)) return false;
        return cmdStartBatch(*static_cast<const domain::IncubationBatch*>(payload));

    case Cmd::StopBatch:
        return cmdStopBatch();

    case Cmd::PatchPlanRow:
        if (!payload || sz != sizeof(domain::IncubationPlanRow)) return false;
        return cmdPatchPlanRow(*static_cast<const domain::IncubationPlanRow*>(payload));

    case Cmd::ResetPlan:
        return cmdResetPlan();

    case Cmd::HeaterOn:
        m_state.safeMode = false;
        m_heater.on();
        m_state.heaterOn = true;
        return true;

    case Cmd::HeaterOff:
        m_heater.off();
        m_state.heaterOn = false;
        return true;

    case Cmd::HumidOn:
        m_humidifier.on();
        m_state.humidifierOn = true;
        return true;

    case Cmd::HumidOff:
        m_humidifier.off();
        m_state.humidifierOn = false;
        return true;

    case Cmd::TurnerOn:
        m_turner.on();
        m_state.turnerOn = true;
        return true;

    case Cmd::TurnerOff:
        m_turner.off();
        m_state.turnerOn = false;
        return true;

    case Cmd::UpdateSettings:
        if (!payload || sz != sizeof(domain::AppSettings)) return false;
        return cmdUpdateSettings(*static_cast<const domain::AppSettings*>(payload));

    case Cmd::ClearSafeMode:
        return cmdClearSafeMode();

    case Cmd::FactoryReset:
        return cmdFactoryReset();

    case Cmd::EnterManualMode:
        return cmdEnterManualMode();

    case Cmd::ExitManualMode:
        return cmdExitManualMode();

    case Cmd::ClearWifiInfo:
        return cmdClearWifiInfo();

    case Cmd::Reboot:
        ESP_LOGW("AppCtrl", "Reboot requested");
        esp_restart();
        return true;

    default:
        ESP_LOGW("AppCtrl", "Unknown Cmd: %u", static_cast<unsigned>(cmd));
        return false;
    }
}

bool AppController::cmdStartBatch(const domain::IncubationBatch& b)
{
    m_batch = b;
    m_batch.active = true;
    if (m_batch.batchId[0] == '\0') generateBatchId(m_batch);
    if (!cmdResetPlan()) return false;
    applyBatchToState();
    if (const auto* row = m_plan.getRow(1)) {
        m_state.targetTempC = row->targetTempC;
        m_state.targetHumidityPct = row->targetHumidityPct;
        m_state.turningEnabled = row->turningEnabled;
    }
    if (!m_batch.isValid()) {
        ESP_LOGE("AppCtrl", "Batch invalid after plan generation");
        m_state.safeMode = true;
        return false;
    }
    if (!m_nvs.saveBlob(storage::NvsStorage::kKeyBatch, &m_batch, sizeof(m_batch))) {
        ESP_LOGE("AppCtrl", "Batch save failed");
        m_state.safeMode = true;
        return false;
    }
    ESP_LOGI("AppCtrl", "Batch started: %s (%s)", m_batch.batchId,
             domain::speciesName(m_batch.species));
    return true;
}

bool AppController::cmdStopBatch()
{
    m_batch.active = false;
    m_nvs.saveBlob(storage::NvsStorage::kKeyBatch, &m_batch, sizeof(m_batch));
    return true;
}

bool AppController::cmdPatchPlanRow(const domain::IncubationPlanRow& row)
{
    auto* r = m_plan.getRowMutable(row.day);
    if (!r) {
        ESP_LOGE("AppCtrl", "PatchPlanRow: day %u not found", row.day);
        return false;
    }
    *r = row;
    r->userOverridden = true;
    m_plan.tableVersion++;
    bool ok = m_planStorage.save(m_plan);
    ESP_LOGI("AppCtrl", "PatchPlanRow: day %u saved (v%u)", (unsigned int)row.day, (unsigned int)m_plan.tableVersion);
    return ok;
}

bool AppController::cmdResetPlan()
{
    m_plan.clear();
    bool ok = policy::PlanGenerator::generate(m_batch.species, m_batch, m_plan);
    if (!ok) {
        ESP_LOGE("AppCtrl", "PlanGenerator failed!");
        m_state.safeMode = true;
        return false;
    }
    if (!m_planStorage.save(m_plan)) {
        ESP_LOGE("AppCtrl", "Plan save failed");
        m_state.safeMode = true;
        return false;
    }
    if (!m_nvs.saveBlob(storage::NvsStorage::kKeyBatch, &m_batch, sizeof(m_batch))) {
        ESP_LOGE("AppCtrl", "Batch save failed during plan reset");
        m_state.safeMode = true;
        return false;
    }
    return true;
}

bool AppController::cmdUpdateSettings(const domain::AppSettings& s)
{
    if (!s.isValid()) {
        ESP_LOGW("AppCtrl", "UpdateSettings: invalid values");
        return false;
    }
    m_settings = s;
    return m_nvs.saveBlob(storage::NvsStorage::kKeySettings, &m_settings, sizeof(m_settings));
}

bool AppController::cmdFactoryReset()
{
    m_nvs.eraseAll();
    m_planStorage.erase();
    m_settings = domain::AppSettings::defaults();
    m_batch = {};
    m_plan.clear();
    m_state = domain::RuntimeState::zero();
    ESP_LOGW("AppCtrl", "Factory reset complete — reboot recommended");
    return true;
}

bool AppController::cmdClearSafeMode()
{
    m_state.safeMode = false;
    ESP_LOGI("AppCtrl", "SafeMode cleared");
    return true;
}

bool AppController::cmdClearWifiInfo()
{
    esp_wifi_restore();
    m_settings.wifiConfigured = false;
    std::memset(m_settings.wifiSsid, 0, sizeof(m_settings.wifiSsid));
    std::memset(m_settings.wifiPassword, 0, sizeof(m_settings.wifiPassword));
    m_nvs.saveBlob(storage::NvsStorage::kKeySettings, &m_settings, sizeof(m_settings));
    ESP_LOGW("AppCtrl", "WiFi info cleared");
    return true;
}

bool AppController::cmdEnterManualMode()
{
    m_state.manualMode = true;
    m_state.safeMode = false;
    m_heater.off();
    m_humidifier.off();
    m_turner.off();
    m_fan.stop();
    m_state.heaterOn = false;
    m_state.humidifierOn = false;
    m_state.turnerOn = false;
    m_state.fanOn = false;
    ESP_LOGW("AppCtrl", "Manual mode entered");
    return true;
}

bool AppController::cmdExitManualMode()
{
    m_heater.off();
    m_humidifier.off();
    m_turner.off();
    m_fan.stop();
    m_state.heaterOn = false;
    m_state.humidifierOn = false;
    m_state.turnerOn = false;
    m_state.fanOn = false;
    m_state.manualMode = false;
    ESP_LOGI("AppCtrl", "Manual mode exited");
    return true;
}

bool AppController::restoreFromStorage()
{
    domain::AppSettings loaded{};
    if (m_nvs.loadBlob(storage::NvsStorage::kKeySettings, &loaded, sizeof(loaded)) && loaded.isValid()) {
        m_settings = loaded;
        ESP_LOGI("AppCtrl", "Settings restored");
    } else {
        m_settings = domain::AppSettings::defaults();
        m_nvs.saveBlob(storage::NvsStorage::kKeySettings, &m_settings, sizeof(m_settings));
        ESP_LOGI("AppCtrl", "Settings: defaults applied");
    }

    uint32_t cnt = 0;
    m_nvs.loadU32(storage::NvsStorage::kKeyBootCount, cnt);
    cnt++;
    m_nvs.saveU32(storage::NvsStorage::kKeyBootCount, cnt);
    m_state.bootCount = cnt;
    ESP_LOGI("AppCtrl", "Boot count: %u", (unsigned int)cnt);

    domain::IncubationBatch b{};
    if (m_nvs.loadBlob(storage::NvsStorage::kKeyBatch, &b, sizeof(b)) && b.isValid()) {
        m_batch = b;
        applyBatchToState();
        ESP_LOGI("AppCtrl", "Batch restored: %s", m_batch.batchId);
    }

    return true;
}

void AppController::validateAndRepairPlan()
{
    if (!m_batch.active) return;

    bool planLoaded = m_planStorage.load(m_plan);
    if (planLoaded && m_plan.isValid()) {
        ESP_LOGI("AppCtrl", "Plan valid: %u rows", m_plan.rowCount);
        return;
    }

    ESP_LOGW("AppCtrl", "Plan missing/invalid — regenerating from Preset");
    m_plan.clear();
    bool ok = policy::PlanGenerator::generate(m_batch.species, m_batch, m_plan);
    if (ok) {
        m_planStorage.save(m_plan);
        ESP_LOGI("AppCtrl", "Plan regenerated: %u rows", m_plan.rowCount);
    } else {
        ESP_LOGE("AppCtrl", "Plan regeneration failed — SafeMode");
        m_state.safeMode = true;
    }
}

void AppController::generateBatchId(domain::IncubationBatch& b)
{
    uint32_t cnt = m_state.bootCount;
    std::snprintf(b.batchId, sizeof(b.batchId), "INC-%03u", static_cast<unsigned>(cnt % 1000));
}

void AppController::applyBatchToState()
{
    m_state.batchActive = m_batch.active;
    m_state.currentDay = 1;
    m_state.totalDays = m_batch.totalDays;
    m_state.lockdownActive = false;
    m_state.lockdownStartDay = m_batch.lockdownStartDay;
    m_state.batchStartEpoch = m_batch.startEpoch;
}

} // namespace incubator::app
