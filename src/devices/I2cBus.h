#pragma once
#include <driver/i2c_master.h> // 최신 헤더로 변경
#include <cstdint>
#include <cstddef>
#include <map>

namespace incubator::devices
{
    class I2cBus
    {
    public:
        static constexpr uint32_t kTimeoutMs = 50U;

        ~I2cBus(); // 소멸자 추가 (리소스 해제용)

        bool init(int sdaPin, int sclPin, uint32_t freqHz = 400000U);
        bool isInitialized() const { return m_initialized; }

        bool write(uint8_t addr, const uint8_t* data, size_t len);
        bool read(uint8_t addr, uint8_t* buf, size_t len);
        bool writeRead(uint8_t addr,
                       const uint8_t* txData, size_t txLen,
                       uint8_t* rxBuf,  size_t rxLen);
        bool isReady(uint8_t addr);

    private:
        // 특정 주소의 장치 핸들을 가져오거나 생성하는 헬퍼 함수
        i2c_master_dev_handle_t getOrCreateDevice(uint8_t addr);

        bool m_initialized = false;
        i2c_master_bus_handle_t m_busHandle = nullptr;
        // 장치별 핸들을 관리하기 위한 맵
        std::map<uint8_t, i2c_master_dev_handle_t> m_devices;
    };
}