#include "devices/GpioOutput.h"
#include <esp_log.h>

namespace incubator::devices
{
    bool GpioOutput::init()
    {
        if (m_pin < 0) {
            ESP_LOGW("GpioOutput", "disabled output pin");
            m_state = false;
            return true;
        }

        gpio_config_t config = {};
        config.mode = GPIO_MODE_OUTPUT;
        config.pin_bit_mask = 1ULL << static_cast<uint32_t>(m_pin);
        config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        config.pull_up_en = GPIO_PULLUP_DISABLE;
        config.intr_type = GPIO_INTR_DISABLE;

        esp_err_t err = gpio_config(&config);
        if (err != ESP_OK) {
            ESP_LOGE("GpioOutput", "gpio_config failed: %s", esp_err_to_name(err));
            return false;
        }
        set(false);
        return true;
    }

    void GpioOutput::set(bool on)
    {
        if (m_pin < 0) {
            m_state = false;
            return;
        }

        uint32_t level = (on ^ m_invert) ? 1U : 0U;
        gpio_set_level(m_pin, static_cast<int>(level));
        m_state = on;
    }
} // namespace incubator::devices
