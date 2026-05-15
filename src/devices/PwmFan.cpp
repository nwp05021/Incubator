#include "devices/PwmFan.h"
#include <esp_log.h>

namespace incubator::devices
{

bool PwmFan::init()
{
    if (m_pin < 0) {
        ESP_LOGW("PwmFan", "disabled PWM pin");
        m_duty = 0;
        return true;
    }

    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode = kMode;
    timer_cfg.duty_resolution = kDutyBit;
    timer_cfg.timer_num = kTimer;
    timer_cfg.freq_hz = m_freq;
    if (ledc_timer_config(&timer_cfg) != ESP_OK) {
        ESP_LOGE("PwmFan", "ledc_timer_config failed");
        return false;
    }

    ledc_channel_config_t ch_cfg = {};
    ch_cfg.gpio_num = m_pin;
    ch_cfg.speed_mode = kMode;
    ch_cfg.channel = m_channel;
    ch_cfg.timer_sel = kTimer;
    ch_cfg.intr_type = LEDC_INTR_DISABLE;
    ch_cfg.duty = 0;
    if (ledc_channel_config(&ch_cfg) != ESP_OK) {
        ESP_LOGE("PwmFan", "ledc_channel_config failed");
        return false;
    }
    setDuty(0);
    return true;
}

void PwmFan::setDuty(uint8_t percent)
{
    if (m_pin < 0) {
        m_duty = 0;
        return;
    }

    if (percent > 100) percent = 100;
    uint32_t duty = (static_cast<uint32_t>(percent) * 255U) / 100U;
    ledc_set_duty(kMode, m_channel, duty);
    ledc_update_duty(kMode, m_channel);
    m_duty = percent;
}

} // namespace incubator::devices
