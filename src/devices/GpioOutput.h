#pragma once
#include <driver/gpio.h>
#include <cstdint>

namespace incubator::devices
{
    class GpioOutput
    {
    public:
        explicit GpioOutput(gpio_num_t pin, bool invertLogic = false)
            : m_pin(pin), m_invert(invertLogic) {}

        bool init();
        void set(bool on);
        bool isOn() const { return m_state; }
        void on() { set(true); }
        void off() { set(false); }
        void toggle() { set(!m_state); }

    private:
        gpio_num_t m_pin;
        bool       m_invert = false;
        bool       m_state  = false;
    };
}
