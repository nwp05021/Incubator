#include "ui/UiController.h"
#include "config/AppConfig.h"
#include "policy/DayResolver.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace incubator::ui
{
namespace
{
    int clampInt(int value, int lo, int hi)
    {
        if (value < lo) return lo;
        if (value > hi) return hi;
        return value;
    }

    bool leap(uint16_t y)
    {
        return ((y % 4U) == 0U && (y % 100U) != 0U) || ((y % 400U) == 0U);
    }

    uint8_t maxDayForMonth(uint16_t year, uint8_t month)
    {
        static constexpr uint8_t kDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        if (month == 2U) return leap(year) ? 29U : 28U;
        if (month < 1U || month > 12U) return 31U;
        return kDays[month - 1U];
    }

    bool dateFromEpoch(uint32_t epoch, uint16_t& year, uint8_t& month, uint8_t& day)
    {
        if (epoch == 0U) return false;
        std::time_t t = static_cast<std::time_t>(epoch);
        std::tm* tmv = std::localtime(&t);
        if (!tmv) return false;
        year = static_cast<uint16_t>(tmv->tm_year + 1900);
        month = static_cast<uint8_t>(tmv->tm_mon + 1);
        day = static_cast<uint8_t>(tmv->tm_mday);
        return year >= 2020U && month >= 1U && month <= 12U && day >= 1U && day <= 31U;
    }
}

void UiController::tick(uint32_t nowMs)
{
    if (nowMs - m_lastSyncMs >= kSyncIntervalMs) {
        syncFromState();
        m_lastSyncMs = nowMs;
    }

    if (m_model.screen == UiScreen::FactoryReset) {
        factoryTick(nowMs);
    }
    handleInput();
}

void UiController::syncFromState()
{
    m_model.displayTempC = m_state.currentTempC;
    m_model.displayHumidPct = m_state.currentHumidityPct;
    m_model.targetTempC = m_state.targetTempC;
    m_model.targetHumidPct = m_state.targetHumidityPct;
    m_model.heaterOn = m_state.heaterOn;
    m_model.humidifierOn = m_state.humidifierOn;
    m_model.turnerOn = m_state.turnerOn;
    m_model.fanOn = m_state.fanOn;
    m_model.tempAlarm = m_state.tempAlarmActive;
    m_model.humiAlarm = m_state.humiAlarmActive;
    m_model.tempSensorFault = !m_state.tempSensorOk;
    m_model.humiSensorFault = !m_state.humiSensorOk;
    m_model.tempSensorWarning = m_state.tempSensorWarning;
    m_model.humiSensorWarning = m_state.humiSensorWarning;
    m_model.currentDay = m_state.currentDay;
    m_model.totalDays = m_state.totalDays;
    m_model.progressPct = policy::DayResolver::progressPct(m_state.currentDay, m_state.totalDays);
    m_model.batchActive = m_state.batchActive;
    m_model.lockdownActive = m_state.lockdownActive;
    m_model.turningEnabled = m_state.turningEnabled;
    m_model.nextTurningInMin = m_state.nextTurningInMin;
    m_model.batchStartEpoch = m_state.batchStartEpoch;
    m_model.safeMode = m_state.safeMode;
    m_model.manualMode = m_state.manualMode;
    m_model.bootCount = m_state.bootCount;
    m_model.uptimeMs = m_state.uptimeMs;
    m_model.cloudConnected = m_state.cloudConnected || m_provisioning.isConnected();
    std::memcpy(m_model.ipAddress, m_state.ipAddress, sizeof(m_model.ipAddress));

    m_model.wifiConfigured = m_provisioning.isConfigured();
    m_model.localMode = m_provisioning.isLocalMode();
    m_model.provisioningActive = m_provisioning.isActive();
    m_model.provisioningSucceeded = m_provisioning.isSucceeded();
    m_model.provisioningFailed = m_provisioning.isFailed();
    uint32_t remaining = m_provisioning.remainingMs(m_state.uptimeMs);
    m_model.provisioningRemainingMs = ((remaining + 999U) / 1000U) * 1000U;
    std::strncpy(m_model.provisioningName, m_provisioning.deviceName(), sizeof(m_model.provisioningName) - 1U);
    std::strncpy(m_model.provisioningPop, m_provisioning.proofOfPossession(), sizeof(m_model.provisioningPop) - 1U);
    std::strncpy(m_model.provisioningMessage, m_provisioning.statusText(), sizeof(m_model.provisioningMessage) - 1U);

    if (m_model.screen == UiScreen::PlanList) {
        refreshPlanList();
    }
}

void UiController::handleInput()
{
    int delta = m_encoder.consumeDelta();
    if (m_provisioning.isActive()) {
        if (m_encoder.wasLongPressed()) handleLongPress();
        else (void)m_encoder.wasPressed();
        return;
    }

    if (delta != 0) handleDelta(delta);
    if (m_encoder.wasLongPressed()) handleLongPress();
    else if (m_encoder.wasPressed()) handleClick();
}

void UiController::handleDelta(int delta)
{
    if (delta == 0) return;
    int step = (delta > 0) ? 1 : -1;

    switch (m_model.screen) {
        case UiScreen::Main:
            m_model.activePage = static_cast<uint8_t>((m_model.activePage + Layout::kMainPageCount + step) % Layout::kMainPageCount);
            break;
        case UiScreen::Menu: {
            int cursor = static_cast<int>(m_model.menuCursor) + step;
            // 🔥 Layout::kMenuCount 대신 kMainMenuCount 사용
            if (cursor < 0) cursor = kMainMenuCount - 1;
            if (cursor >= kMainMenuCount) cursor = 0;
            m_model.menuCursor = static_cast<uint8_t>(cursor);
            break;
        }
        case UiScreen::StartDate: startDateDelta(step); break;
        case UiScreen::Preset: presetDelta(step); break;
        case UiScreen::PlanList: planListDelta(step); break;
        case UiScreen::PlanEdit: page3Delta(step); break;
        case UiScreen::Manual: page2Delta(step); break;
        case UiScreen::AwsTest: awsTestDelta(step); break;
        case UiScreen::WifiReset:
        case UiScreen::RebootConfirm:
            m_model.confirmCursor = m_model.confirmCursor ? 0 : 1;
            break;
        default:
            break;
    }
}

void UiController::handleClick()
{
    switch (m_model.screen) {
        case UiScreen::Main:
            // 추가된 부분: activePage가 2(Help/Test)일 때 클릭 처리
            if (m_model.activePage == 2) {
                if (!m_model.cloudConnected) {
                    m_model.pushAwsLog("실패: 네트워크 미연결");
                } else if (!m_model.awsTestRunning) {
                    m_model.awsTestRunning = true;
                    m_model.pushAwsLog("1. 테스트 시작 요청...");
                    // AppController를 통해 실제 AWS 전송 명령 하달 (Command Pattern)
                    //m_ctrl.applyCommand(app::Cmd::TestAwsIot); 
                }
            }
            break;        
        case UiScreen::Menu: enterMenuItem(); break;
        case UiScreen::StartDate: startDateClick(); break;
        case UiScreen::Preset: presetClick(); break;
        case UiScreen::PlanList:
            m_model.screen = UiScreen::PlanEdit;
            m_model.fieldCursor = 0;
            m_model.editMode = false;
            m_model.activeEditField = EditField::Temp;
            loadEditRow(m_model.editDay);
            break;
        case UiScreen::PlanEdit: page3Click(); break;
        case UiScreen::Manual: page2Click(); break;
        case UiScreen::AwsTest: awsTestClick(); break;
        case UiScreen::WifiReset: wifiResetClick(); break;
        case UiScreen::RebootConfirm: rebootClick(); break;
        default:
            break;
    }
}

void UiController::handleLongPress()
{
    if (m_provisioning.isActive()) {
        m_provisioning.cancel();
        goHome();
        return;
    }

    switch (m_model.screen) {
        case UiScreen::Main:
            goMenu();
            break;
        case UiScreen::PlanEdit:
            m_model.editMode = false;
            m_model.screen = UiScreen::PlanList;
            break;
        case UiScreen::Manual:
            exitManual();
            goMenu();
            break;
        case UiScreen::FactoryReset:
            goMenu();
            break;
        case UiScreen::Menu:
            goHome();
            break;
        default:
            goMenu();
            break;
    }
}

void UiController::enterMenuItem()
{
    m_model.actionMessage[0] = '\0';
    
    // 🔥 현재 커서가 가리키는 메뉴 항목의 '목표 화면(Target Screen)'을 가져옵니다.
    UiScreen target = kMainMenuItems[m_model.menuCursor].targetScreen;

    // 🔥 인덱스가 아닌 '목표 화면'을 기준으로 동작을 분기합니다!
    switch (target) {
        case UiScreen::StartDate:
            initDateFromSavedOrNow();
            m_model.fieldCursor = 0;
            m_model.editMode = false;
            m_model.screen = target;
            break;
        case UiScreen::Preset:
            m_model.presetConfirm = false;
            m_model.confirmCursor = 1;
            m_model.screen = target;
            break;
        case UiScreen::PlanList:
            m_model.screen = target;
            if (m_model.editDay == 0) m_model.editDay = 1;
            loadEditRow(m_model.editDay);
            refreshPlanList();
            break;
        case UiScreen::Manual:
            enterManual();
            break;
        case UiScreen::AwsTest:
            m_model.confirmCursor = 0;
            m_model.awsTestRunning = false;
            m_model.screen = target;
            break;
        case UiScreen::WifiReset:
            m_model.confirmCursor = 0;
            m_model.screen = target;
            break;
        case UiScreen::BleProvisioning:
            m_provisioning.requestMenuProvisioning(m_state.uptimeMs);
            goHome();
            break;
        case UiScreen::RebootConfirm:
            m_model.confirmCursor = 0;
            m_model.screen = target;
            break;
        case UiScreen::FactoryReset:
            m_model.factoryProgressPct = 0;
            m_model.factoryReady = false;
            m_factoryStartedMs = 0;
            m_model.screen = target;
            break;
        default:
            break;
    }
}

void UiController::enterManual()
{
    m_ctrl.applyCommand(app::Cmd::EnterManualMode);
    m_model.manualCursor = 0;
    m_model.screen = UiScreen::Manual;
}

void UiController::exitManual()
{
    m_ctrl.applyCommand(app::Cmd::ExitManualMode);
}

void UiController::startDateDelta(int d)
{
    if (!m_model.editMode) {
        int c = static_cast<int>(m_model.fieldCursor) + d;
        m_model.fieldCursor = static_cast<uint8_t>(clampInt(c, 0, 4));
        return;
    }

    switch (m_model.fieldCursor) {
        case 0:
            m_model.editBatchYear = static_cast<uint16_t>(clampInt(static_cast<int>(m_model.editBatchYear) + d, 2020, 2099));
            break;
        case 1: {
            int m = static_cast<int>(m_model.editBatchMonth) + d;
            if (m < 1) m = 12;
            if (m > 12) m = 1;
            m_model.editBatchMonth = static_cast<uint8_t>(m);
            break;
        }
        case 2:
            m_model.editBatchDay = static_cast<uint8_t>(clampInt(static_cast<int>(m_model.editBatchDay) + d, 1,
                                          maxDayForMonth(m_model.editBatchYear, m_model.editBatchMonth)));
            break;
    }
    m_model.editBatchDay = std::min<uint8_t>(m_model.editBatchDay,
                                             maxDayForMonth(m_model.editBatchYear, m_model.editBatchMonth));
}

void UiController::startDateClick()
{
    if (m_model.fieldCursor <= 2U) {
        m_model.editMode = !m_model.editMode;
        if (!m_model.editMode && m_model.fieldCursor < 4U) ++m_model.fieldCursor;
        return;
    }

    if (m_model.fieldCursor == 3U) {
        domain::IncubationBatch batch;
        batch.species = domain::Species::Chicken;
        batch.startEpoch = editDateEpoch();
        if (m_ctrl.applyCommand(app::Cmd::StartBatch, &batch, sizeof(batch))) {
            m_model.batchStartEpoch = batch.startEpoch;
            std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage), "시작일 저장");
            goMenu();
        } else {
            std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage), "저장 실패");
        }
        return;
    }
    goMenu();
}

void UiController::presetDelta(int d)
{
    if (m_model.presetConfirm) {
        m_model.confirmCursor = m_model.confirmCursor ? 0 : 1;
        return;
    }
    int c = static_cast<int>(m_model.presetCursor) + d;
    if (c < 0) c = 3;
    if (c > 3) c = 0;
    m_model.presetCursor = static_cast<uint8_t>(c);
}

void UiController::presetClick()
{
    if (!m_model.presetConfirm) {
        m_model.presetConfirm = true;
        m_model.confirmCursor = 1;
        return;
    }
    if (m_model.confirmCursor == 1U) {
        startBatchFromPreset(static_cast<domain::Species>(m_model.presetCursor));
        if (m_model.editDay == 0U) m_model.editDay = 1U;
        loadEditRow(m_model.editDay);
    }
    goMenu();
}

void UiController::planListDelta(int d)
{
    int day = static_cast<int>(m_model.editDay) + d;
    int maxDay = (m_plan.rowCount > 0U) ? m_plan.rowCount : m_model.totalDays;
    m_model.editDay = static_cast<uint16_t>(clampInt(day, 1, maxDay));
    loadEditRow(m_model.editDay);
    refreshPlanList();
}

void UiController::awsTestDelta(int d)
{
    if (m_model.awsTestRunning || kAwsPublishEndpointCount == 0U) return;

    int cursor = static_cast<int>(m_model.awsEndpointCursor) + d;
    if (cursor < 0) cursor = kAwsPublishEndpointCount - 1;
    if (cursor >= kAwsPublishEndpointCount) cursor = 0;
    m_model.awsEndpointCursor = static_cast<uint8_t>(cursor);
}

void UiController::awsTestClick()
{
    if (m_model.awsTestRunning) return;

    if (!m_model.cloudConnected) {
        m_model.awsLastResult = false;
        m_model.pushAwsLog("FAIL: MQTT disconnected");
        return;
    }
    if (!m_awsPublishCb) {
        m_model.awsLastResult = false;
        m_model.pushAwsLog("FAIL: publisher missing");
        return;
    }

    char topic[128];
    char payload[512];
    formatAwsTopic(topic, sizeof(topic));
    if (!buildAwsTestPayload(payload, sizeof(payload))) {
        m_model.awsLastResult = false;
        m_model.pushAwsLog("FAIL: payload build");
        return;
    }

    m_model.awsTestRunning = true;
    m_model.pushAwsLog("Publish requested");
    bool ok = m_awsPublishCb(topic, payload);
    m_model.awsTestRunning = false;
    m_model.awsLastResult = ok;
    ++m_model.awsTestCount;

    char log[64];
    std::snprintf(log, sizeof(log), "%s %s",
                  ok ? "OK:" : "FAIL:",
                  kAwsPublishEndpoints[m_model.awsEndpointCursor].name);
    m_model.pushAwsLog(log);
}

bool UiController::buildAwsTestPayload(char* buf, size_t bufSize) const
{
    if (!buf || bufSize == 0U) return false;

    const AwsPublishEndpoint& endpoint = kAwsPublishEndpoints[m_model.awsEndpointCursor];
    const char* batchId = m_model.batchId[0] ? m_model.batchId : "";

    int written = std::snprintf(
        buf, bufSize,
        "{\"test\":true,"
        "\"endpoint\":\"%s\","
        "\"seq\":%u,"
        "\"ts\":%u,"
        "\"uptimeMs\":%u,"
        "\"sensor\":{\"tempC\":%.2f,\"humidityPct\":%.2f},"
        "\"target\":{\"tempC\":%.2f,\"humidityPct\":%.2f},"
        "\"state\":{\"day\":%u,\"totalDays\":%u,\"batchActive\":%s,\"safeMode\":%s},"
        "\"actuator\":{\"heater\":%s,\"humidifier\":%s,\"turner\":%s,\"fan\":%s},"
        "\"batchId\":\"%s\"}",
        endpoint.name,
        static_cast<unsigned>(m_model.awsTestCount + 1U),
        static_cast<unsigned>(std::time(nullptr)),
        static_cast<unsigned>(m_model.uptimeMs),
        static_cast<double>(m_model.displayTempC),
        static_cast<double>(m_model.displayHumidPct),
        static_cast<double>(m_model.targetTempC),
        static_cast<double>(m_model.targetHumidPct),
        static_cast<unsigned>(m_model.currentDay),
        static_cast<unsigned>(m_model.totalDays),
        m_model.batchActive ? "true" : "false",
        m_model.safeMode ? "true" : "false",
        m_model.heaterOn ? "true" : "false",
        m_model.humidifierOn ? "true" : "false",
        m_model.turnerOn ? "true" : "false",
        m_model.fanOn ? "true" : "false",
        batchId);

    if (written < 0) return false;
    if (static_cast<size_t>(written) >= bufSize) {
        buf[bufSize - 1U] = '\0';
        return false;
    }
    return true;
}

void UiController::formatAwsTopic(char* buf, size_t bufSize) const
{
    if (!buf || bufSize == 0U) return;

#ifdef INCUBATOR_ENABLE_CLOUD
    const char* deviceId = INCUBATOR_DEVICE_ID;
#else
    const char* deviceId = "Incubator-TEST";
#endif

    std::snprintf(buf, bufSize,
                  kAwsPublishEndpoints[m_model.awsEndpointCursor].topicFormat,
                  deviceId);
    buf[bufSize - 1U] = '\0';
}

void UiController::wifiResetClick()
{
    if (m_model.confirmCursor == 1U) {
        m_ctrl.applyCommand(app::Cmd::ClearWifiInfo);
        std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage), "WiFi 정보 삭제");
    }
    goMenu();
}

void UiController::rebootClick()
{
    if (m_model.confirmCursor == 1U) {
        m_ctrl.applyCommand(app::Cmd::Reboot);
    }
    goMenu();
}

void UiController::factoryTick(uint32_t nowMs)
{
    if (m_encoder.isButtonDown()) {
        if (m_factoryStartedMs == 0U) m_factoryStartedMs = nowMs;
        uint32_t held = nowMs - m_factoryStartedMs;
        if (held >= 10000U) {
            m_model.factoryProgressPct = 100;
            m_ctrl.applyCommand(app::Cmd::FactoryReset);
            goHome();
        } else {
            m_model.factoryProgressPct = static_cast<uint8_t>((held * 100U) / 10000U);
        }
    } else if (m_factoryStartedMs != 0U) {
        m_factoryStartedMs = 0U;
        m_model.factoryProgressPct = 0U;
    }
}

void UiController::startBatchFromPreset(domain::Species species)
{
    domain::IncubationBatch batch;
    batch.species = species;
    batch.startEpoch = editDateEpoch();
    if (m_ctrl.applyCommand(app::Cmd::StartBatch, &batch, sizeof(batch))) {
        m_model.totalDays = m_plan.rowCount;
        m_model.currentDay = 1;
        std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage), "프리셋 적용");
    }
}

void UiController::goMenu()
{
    m_model.screen = UiScreen::Menu;
    m_model.menuOpen = true;
    m_model.activeEditField = EditField::None;
    m_model.editMode = false;
}

void UiController::goHome()
{
    m_model.screen = UiScreen::Main;
    m_model.menuOpen = false;
    m_model.activePage = 0;
    m_model.activeEditField = EditField::None;
    m_model.editMode = false;
}

void UiController::page0Delta(int)
{
}

void UiController::page1Delta(int)
{
}

void UiController::page2Delta(int d)
{
    int cursor = m_model.manualCursor + d;
    m_model.manualCursor = static_cast<int8_t>(clampInt(cursor, 0, 3));
}

void UiController::page2Click()
{
    switch (m_model.manualCursor) {
        case 0: m_ctrl.applyCommand(m_model.heaterOn ? app::Cmd::HeaterOff : app::Cmd::HeaterOn); break;
        case 1: m_ctrl.applyCommand(m_model.humidifierOn ? app::Cmd::HumidOff : app::Cmd::HumidOn); break;
        case 2: m_ctrl.applyCommand(m_model.turnerOn ? app::Cmd::TurnerOff : app::Cmd::TurnerOn); break;
        case 3: {
            uint8_t duty = m_model.fanOn ? 0U : 50U;
            m_ctrl.applyCommand(app::Cmd::FanSetDuty, &duty, sizeof(duty));
            break;
        }
    }
}

void UiController::page3Delta(int d)
{
    if (!m_model.editMode) {
        int cursor = static_cast<int>(m_model.fieldCursor) + d;
        m_model.fieldCursor = static_cast<uint8_t>(clampInt(cursor, 0, 5));
        switch (m_model.fieldCursor) {
            case 0: m_model.activeEditField = EditField::Temp; break;
            case 1: m_model.activeEditField = EditField::Humidity; break;
            case 2: m_model.activeEditField = EditField::Turning; break;
            default: m_model.activeEditField = EditField::None; break;
        }
        return;
    }

    if (m_model.fieldCursor == 0U) {
        int tempQ10 = static_cast<int>(std::lroundf(m_model.editTempC * 10.0f)) + d;
        m_model.editTempC = static_cast<float>(clampInt(tempQ10, 200, 450)) / 10.0f;
    } else if (m_model.fieldCursor == 1U) {
        m_model.editHumidPct = static_cast<float>(clampInt(static_cast<int>(m_model.editHumidPct + d), 30, 95));
    } else if (m_model.fieldCursor == 2U) {
        m_model.editTurning = !m_model.editTurning;
    } else if (m_model.fieldCursor == 3U) {
        m_model.editIntervalMin = static_cast<uint16_t>(clampInt(static_cast<int>(m_model.editIntervalMin) + d * 10, 30, 480));
    }
}

void UiController::page3Click()
{
    if (m_model.fieldCursor <= 3U) {
        m_model.editMode = !m_model.editMode;
        return;
    }

    if (m_model.fieldCursor == 4U) {
        savePlanEdit();
        m_model.editMode = false;
        m_model.screen = UiScreen::PlanList;
    } else if (m_model.fieldCursor == 5U) {
        loadEditRow(m_model.editDay);
        m_model.editMode = false;
        m_model.screen = UiScreen::PlanList;
    }
}

void UiController::savePlanEdit()
{
    domain::IncubationPlanRow row;
    row.day = m_model.editDay;
    row.targetTempC = m_model.editTempC;
    row.targetHumidityPct = m_model.editHumidPct;
    row.turningEnabled = m_model.editTurning;
    row.turningIntervalMin = m_model.editIntervalMin;
    row.ventFanEnabled = true;
    row.userOverridden = true;
    if (m_ctrl.applyCommand(app::Cmd::PatchPlanRow, &row, sizeof(row))) {
        m_model.activeEditField = EditField::None;
        loadEditRow(m_model.editDay);
        refreshPlanList();
        std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage), "Day %u 저장", m_model.editDay);
    }
}

void UiController::loadEditRow(uint16_t day)
{
    if (day == 0) return;
    const auto* row = m_plan.getRow(day);
    if (!row) return;
    m_model.editDay = row->day;
    m_model.editTempC = row->targetTempC;
    m_model.editHumidPct = row->targetHumidityPct;
    m_model.editTurning = row->turningEnabled;
    m_model.editIntervalMin = row->turningIntervalMin;
    m_model.editOverridden = row->userOverridden;
}

void UiController::refreshPlanList()
{
    m_model.planListCount = 0;
    if (m_plan.rowCount == 0U) return;

    uint16_t first = (m_model.editDay > 4U) ? static_cast<uint16_t>(m_model.editDay - 3U) : 1U;
    for (uint8_t i = 0; i < 5; ++i) {
        uint16_t day = static_cast<uint16_t>(first + i);
        const auto* row = m_plan.getRow(day);
        if (!row) break;

        auto& item = m_model.planList[m_model.planListCount++];
        item.day = row->day;
        item.targetTempC = row->targetTempC;
        item.targetHumidPct = row->targetHumidityPct;
        item.turningEnabled = row->turningEnabled;
        item.intervalMin = row->turningIntervalMin;
        item.overridden = row->userOverridden;
    }
}

uint32_t UiController::editDateEpoch() const
{
    std::tm tmv = {};
    tmv.tm_year = static_cast<int>(m_model.editBatchYear) - 1900;
    tmv.tm_mon = static_cast<int>(m_model.editBatchMonth) - 1;
    tmv.tm_mday = static_cast<int>(m_model.editBatchDay);
    tmv.tm_hour = 0;
    tmv.tm_min = 0;
    tmv.tm_sec = 0;
    std::time_t t = std::mktime(&tmv);
    return (t < 0) ? 0U : static_cast<uint32_t>(t);
}

void UiController::initDateFromSavedOrNow()
{
    if (dateFromEpoch(m_model.batchStartEpoch, m_model.editBatchYear, m_model.editBatchMonth, m_model.editBatchDay)) {
        return;
    }

    std::time_t now = std::time(nullptr);
    std::tm* tmv = std::localtime(&now);
    if (tmv) {
        m_model.editBatchYear = static_cast<uint16_t>(tmv->tm_year + 1900);
        m_model.editBatchMonth = static_cast<uint8_t>(tmv->tm_mon + 1);
        m_model.editBatchDay = static_cast<uint8_t>(tmv->tm_mday);
    }
}

} // namespace incubator::ui
