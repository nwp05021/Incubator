#pragma once
#include <driver/ledc.h>
#include <cstdint>

namespace incubator::devices
{
    class PwmFan
    {
    public:
        PwmFan(int pin, ledc_channel_t channel, uint32_t freqHz = 25000U)
            : m_pin(pin), m_channel(channel), m_freq(freqHz) {}

        bool init();
        void setDuty(uint8_t percent);
        uint8_t getDuty() const { return m_duty; }
        void stop() { setDuty(0); }

    private:
        int            m_pin;
        ledc_channel_t m_channel;
        uint32_t       m_freq;
        uint8_t        m_duty = 0;

        static constexpr ledc_timer_t    kTimer     = LEDC_TIMER_0;
        static constexpr ledc_mode_t     kMode      = LEDC_LOW_SPEED_MODE;
        static constexpr ledc_timer_bit_t kDutyBit  = LEDC_TIMER_8_BIT;
    };
}
