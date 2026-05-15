#pragma once
#include "domain/AppSettings.h"
#include "domain/RuntimeState.h"
#include "domain/IncubationBatch.h"
#include "domain/IncubationPlanTable.h"
#include "domain/IncubationPlanRow.h"
#include "domain/IncubationSpecies.h"
#include "devices/GpioOutput.h"
#include "devices/PwmFan.h"
#include "storage/NvsStorage.h"
#include "storage/PlanStorage.h"
#include "policy/PlanGenerator.h"
#include <cstdint>
#include <cstddef>

namespace incubator::app
{
    enum class Cmd : uint16_t
    {
        StartBatch      = 100,
        StopBatch       = 101,
        PatchPlanRow    = 200,
        ResetPlan       = 201,
        HeaterOn        = 300,
        HeaterOff       = 301,
        HumidOn         = 302,
        HumidOff        = 303,
        TurnerOn        = 304,
        TurnerOff       = 305,
        FanSetDuty      = 306,
        UpdateSettings  = 400,
        ClearSafeMode   = 500,
        FactoryReset    = 501,
        SyncNow         = 502,
        EnterManualMode = 600,
        ExitManualMode  = 601,
        ClearWifiInfo   = 700,
        Reboot          = 800,
    };

    class AppController
    {
    public:
        AppController(domain::RuntimeState&        state,
                      domain::AppSettings&         settings,
                      domain::IncubationBatch&     batch,
                      domain::IncubationPlanTable& plan,
                      storage::NvsStorage&         nvs,
                      storage::PlanStorage&        planStorage,
                      devices::GpioOutput&          heater,
                      devices::GpioOutput&          humidifier,
                      devices::GpioOutput&          turner,
                      devices::PwmFan&              fan)
            : m_state(state), m_settings(settings),
              m_batch(batch), m_plan(plan),
              m_nvs(nvs), m_planStorage(planStorage),
              m_heater(heater), m_humidifier(humidifier), m_turner(turner),
              m_fan(fan) {}

        bool applyCommand(Cmd cmd,
                          const void* payload = nullptr,
                          size_t      payloadSz = 0);

        bool restoreFromStorage();
        void validateAndRepairPlan();

    private:
        domain::RuntimeState&        m_state;
        domain::AppSettings&         m_settings;
        domain::IncubationBatch&     m_batch;
        domain::IncubationPlanTable& m_plan;
        storage::NvsStorage&         m_nvs;
        storage::PlanStorage&        m_planStorage;
        devices::GpioOutput&         m_heater;
        devices::GpioOutput&         m_humidifier;
        devices::GpioOutput&         m_turner;
        devices::PwmFan&            m_fan;

        bool cmdStartBatch(const domain::IncubationBatch& b);
        bool cmdStopBatch();
        bool cmdPatchPlanRow(const domain::IncubationPlanRow& row);
        bool cmdResetPlan();
        bool cmdUpdateSettings(const domain::AppSettings& s);
        bool cmdFactoryReset();
        bool cmdClearWifiInfo();
        bool cmdEnterManualMode();
        bool cmdExitManualMode();
        bool cmdClearSafeMode();

        void generateBatchId(domain::IncubationBatch& b);
        void applyBatchToState();
    };
}
