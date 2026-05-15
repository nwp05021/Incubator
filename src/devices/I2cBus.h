#pragma once
#include <driver/i2c.h>
#include <cstdint>
#include <cstddef>

namespace incubator::devices
{
    class I2cBus
    {
    public:
        static constexpr uint32_t kTimeoutMs = 50U;

        bool init(int sdaPin, int sclPin, uint32_t freqHz = 400000U);
        bool isInitialized() const { return m_initialized; }

        bool write(uint8_t addr, const uint8_t* data, size_t len);
        bool read(uint8_t addr, uint8_t* buf, size_t len);
        bool writeRead(uint8_t addr,
                       const uint8_t* txData, size_t txLen,
                       uint8_t*       rxBuf,  size_t rxLen);
        bool isReady(uint8_t addr);

    private:
        bool   m_initialized = false;
        i2c_port_t m_port = I2C_NUM_0;
    };
}
