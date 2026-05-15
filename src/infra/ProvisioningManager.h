#pragma once

#include "domain/AppSettings.h"
#include "storage/NvsStorage.h"
#include <cstdint>

namespace incubator::infra
{
    enum class ProvisioningState : uint8_t
    {
        Idle,
        Advertising,
        Saving,
        Succeeded,
        TimedOut,
        LocalMode,
        Failed,
    };

    class ProvisioningManager
    {
    public:
        ProvisioningManager(domain::AppSettings& settings, storage::NvsStorage& nvs)
            : m_settings(settings), m_nvs(nvs) {}

        void init();
        void startBootProvisioning(uint32_t nowMs);
        void requestMenuProvisioning(uint32_t nowMs);
        void cancel();
        void tick(uint32_t nowMs);

        bool isActive() const { return m_active; }
        bool isConnected() const;
        bool isConfigured() const { return m_settings.wifiConfigured; }
        bool isLocalMode() const { return m_state == ProvisioningState::LocalMode; }
        bool isSucceeded() const { return m_state == ProvisioningState::Succeeded; }
        bool isAdvertising() const { return m_state == ProvisioningState::Advertising; }
        bool isFailed() const { return m_state == ProvisioningState::Failed || m_state == ProvisioningState::TimedOut; }
        uint32_t remainingMs(uint32_t nowMs) const;
        ProvisioningState state() const { return m_state; }
        void handleEvent(int32_t eventId, void* eventInfo);

        const char* deviceName() const { return m_deviceName; }
        const char* proofOfPossession() const { return m_pop; }
        const char* statusText() const;

    private:
        void start(uint32_t nowMs, uint32_t timeoutMs, bool resetProvisioned);
        void stopProvisioning();
        void buildDeviceIdentity();
        void markConnected();
        void startTimeSync();

        domain::AppSettings& m_settings;
        storage::NvsStorage& m_nvs;
        ProvisioningState m_state = ProvisioningState::Idle;
        bool m_active = false;
        bool m_startedStack = false;
        uint32_t m_startedAtMs = 0;
        uint32_t m_timeoutMs = 0;
        uint32_t m_lastConnectLogMs = 0;
        uint32_t m_finishAtMs = 0;
        bool m_timeSyncStarted = false;
        char m_deviceName[32] = {};
        char m_pop[16] = "incubator";
    };
}
