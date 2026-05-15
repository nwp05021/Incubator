#pragma once
#include <cstdint>

namespace incubator::ui
{
    enum class EditField : uint8_t { None, Day, Temp, Humidity, Turning };
    enum class UiScreen : uint8_t
    {
        Main,
        Menu,
        StartDate,
        Preset,
        PlanList,
        PlanEdit,
        Manual,
        WifiReset,
        RebootConfirm,
        FactoryReset,
        System,
    };

    struct PlanListItem
    {
        uint16_t day = 0;
        float    targetTempC = 0.0f;
        float    targetHumidPct = 0.0f;
        bool     turningEnabled = false;
        uint16_t intervalMin = 0;
        bool     overridden = false;
    };

    struct UiModel
    {
        float    displayTempC      = 0.0f;
        float    displayHumidPct   = 0.0f;
        float    targetTempC       = 37.5f;
        float    targetHumidPct    = 55.0f;
        bool     heaterOn          = false;
        bool     humidifierOn      = false;
        bool     turnerOn          = false;
        bool     fanOn             = false;
        bool     tempAlarm         = false;
        bool     humiAlarm         = false;
        bool     tempSensorFault   = false;
        bool     humiSensorFault   = false;
        bool     tempSensorWarning = false;
        bool     humiSensorWarning = false;
        uint16_t currentDay        = 0;
        uint16_t totalDays         = 21;
        uint8_t  progressPct       = 0;
        bool     batchActive       = false;
        bool     lockdownActive    = false;
        bool     turningEnabled    = true;
        uint16_t nextTurningInMin  = 0;
        uint16_t lockdownStartDay  = 19;
        uint32_t batchStartEpoch   = 0;

        uint16_t editDay           = 1;
        float    editTempC         = 37.5f;
        float    editHumidPct      = 55.0f;
        bool     editTurning       = true;
        uint16_t editIntervalMin   = 120;
        bool     editOverridden    = false;
        EditField activeEditField  = EditField::None;

        uint32_t bootCount         = 0;
        uint32_t uptimeMs          = 0;
        bool     cloudConnected    = false;
        bool     manualMode        = false;
        char     ipAddress[16]     = {};
        char     batchId[16]       = {};
        char     fwVersion[12]     = "v1.0.0";

        UiScreen screen            = UiScreen::Main;
        uint8_t  activePage        = 0;
        bool     menuOpen          = false;
        uint8_t  menuCursor        = 0;
        bool     safeMode          = false;
        int8_t   manualCursor      = 0;
        uint8_t  confirmCursor     = 0;
        uint8_t  presetCursor      = 0;
        bool     presetConfirm     = false;
        bool     editMode          = false;
        uint8_t  fieldCursor       = 0;
        uint8_t  planListCount     = 0;
        PlanListItem planList[5]   = {};
        uint16_t editBatchYear     = 2026;
        uint8_t  editBatchMonth    = 1;
        uint8_t  editBatchDay      = 1;
        uint8_t  factoryProgressPct = 0;
        bool     factoryReady      = false;
        char     actionMessage[40] = {};

        bool     wifiConfigured    = false;
        bool     localMode         = false;
        bool     provisioningActive = false;
        bool     provisioningSucceeded = false;
        bool     provisioningFailed = false;
        uint32_t provisioningRemainingMs = 0;
        char     provisioningName[32] = {};
        char     provisioningPop[16] = {};
        char     provisioningMessage[40] = {};
    };
}
