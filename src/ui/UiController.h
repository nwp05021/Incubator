#pragma once
#include "ui/UiLayout.h"
#include "ui/UiModel.h"
#include "domain/RuntimeState.h"
#include "domain/IncubationPlanTable.h"
#include "app/AppController.h"
#include "devices/Ec11Encoder.h"
#include "infra/ProvisioningManager.h"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>

namespace incubator::ui
{
    class UiController
    {
    public:
        static constexpr uint32_t kSyncIntervalMs = 100;
        using AwsPublishCallback = std::function<bool(const char* topic, const char* payload)>;

        UiController(UiModel& model,
                     const domain::RuntimeState& state,
                     const domain::IncubationPlanTable& plan,
                     app::AppController& ctrl,
                     infra::ProvisioningManager& provisioning,
                     devices::Ec11Encoder& encoder)
            : m_model(model), m_state(state), m_plan(plan), m_ctrl(ctrl),
              m_provisioning(provisioning), m_encoder(encoder)
        {
            m_model.activePage = 0;
        }

        void tick(uint32_t nowMs);
        void setAwsPublishCallback(AwsPublishCallback cb) { m_awsPublishCb = cb; }

    private:
        void syncFromState();
        void handleInput();
        void handleDelta(int delta);
        void handleClick();
        void handleLongPress();
        void page2Delta(int d);
        void page2Click();
        void page3Delta(int d);
        void page3Click();
        void enterMenuItem();
        void enterManual();
        void exitManual();
        void startDateDelta(int d);
        void startDateClick();
        void presetDelta(int d);
        void presetClick();
        void planListDelta(int d);
        void awsTestDelta(int d);
        void awsTestClick();
        bool buildAwsTestPayload(char* buf, size_t bufSize) const;
        void formatAwsTopic(char* buf, size_t bufSize) const;
        void wifiResetClick();
        void rebootClick();
        void factoryTick(uint32_t nowMs);
        void startBatchFromPreset(domain::Species species);
        void goMenu();
        void goHome();
        void savePlanEdit();
        void loadEditRow(uint16_t day);
        void refreshPlanList();
        uint32_t editDateEpoch() const;
        void initDateFromSavedOrNow();

        UiModel& m_model;
        const domain::RuntimeState& m_state;
        const domain::IncubationPlanTable& m_plan;
        app::AppController& m_ctrl;
        infra::ProvisioningManager& m_provisioning;
        devices::Ec11Encoder& m_encoder;
        AwsPublishCallback m_awsPublishCb;

        uint32_t m_lastSyncMs = 0;
        uint32_t m_factoryStartedMs = 0;
    };
}
