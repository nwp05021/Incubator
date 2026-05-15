#pragma once
#include "I2cBus.h"
#include <cstdint>

namespace incubator::devices
{
    struct Aht20Reading
    {
        float    tempC       = 0.0f;
        float    humidityPct = 0.0f;
        bool     valid       = false;
        uint32_t timestampMs = 0U;
    };

    class Aht20Driver
    {
    public:
        static constexpr uint8_t  kAddress         = 0x38U;
        static constexpr uint32_t kMeasureDelayMs  = 80U;

        explicit Aht20Driver(I2cBus& bus) : m_bus(bus) {}

        bool init();

        bool triggerMeasurement();
        bool fetchResult();

        float    getCachedTemp()      const { return m_cached.tempC; }
        float    getCachedHumi()      const { return m_cached.humidityPct; }
        bool     isCacheValid()       const { return m_cached.valid; }
        uint32_t getCacheTimestamp()  const { return m_cached.timestampMs; }
        bool     isConnected()        const { return m_ok; }

        bool reinitialize();

    private:
        I2cBus&      m_bus;
        Aht20Reading m_cached = {};
        bool         m_ok = false;
        bool         m_measurePending = false;
        uint32_t     m_triggerMs = 0;
    };
}
