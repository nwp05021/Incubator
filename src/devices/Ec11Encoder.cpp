#include "devices/Ec11Encoder.h"
#include <esp_log.h>
#include <driver/gpio.h>

namespace incubator::devices
{

static const int8_t kTable[4][4] = {
    { 0, -1,  1,  0},
    { 1,  0,  0, -1},
    {-1,  0,  0,  1},
    { 0,  1, -1,  0}
};

bool Ec11Encoder::init()
{
    gpio_config_t cfg = {};
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pin_bit_mask = (1ULL << m_pinA) | (1ULL << m_pinB);
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&cfg) != ESP_OK) {
        ESP_LOGE("Ec11Encoder", "gpio_config failed for encoder pins");
        return false;
    }

    gpio_config_t btn_cfg = {};
    btn_cfg.mode = GPIO_MODE_INPUT;
    btn_cfg.pin_bit_mask = 1ULL << m_pinBtn;
    btn_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    btn_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    btn_cfg.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&btn_cfg) != ESP_OK) {
        ESP_LOGE("Ec11Encoder", "gpio_config failed for button");
        return false;
    }

    int a = gpio_get_level(static_cast<gpio_num_t>(m_pinA));
    int b = gpio_get_level(static_cast<gpio_num_t>(m_pinB));
    m_lastState = (a << 1) | b;
    m_detentState = m_lastState;
    return true;
}

int Ec11Encoder::consumeDelta()
{
    int d = m_delta;
    m_delta = 0;
    return d;
}

void Ec11Encoder::tick(uint32_t nowMs)
{
    pollEncoder(nowMs);

    bool raw = gpio_get_level(static_cast<gpio_num_t>(m_pinBtn)) == 0;
    if (raw != m_btnRaw) {
        if (nowMs - m_lastBounceMs >= kDebounceMs) {
            m_lastBounceMs = nowMs;
            m_btnRaw = raw;
            if (raw) {
                m_btnDownMs = nowMs;
                m_waitRelease = true;
            } else if (m_waitRelease) {
                uint32_t pressedMs = nowMs - m_btnDownMs;
                if (pressedMs >= kLongPressMs) {
                    m_btnLongPress = true;
                } else {
                    m_btnPressed = true;
                }
                m_waitRelease = false;
            }
        }
    }
}

bool Ec11Encoder::wasPressed()
{
    if (!m_btnPressed) return false;
    m_btnPressed = false;
    return true;
}

bool Ec11Encoder::wasLongPressed()
{
    if (!m_btnLongPress) return false;
    m_btnLongPress = false;
    return true;
}

void Ec11Encoder::pollEncoder(uint32_t nowMs)
{
    int a = gpio_get_level(static_cast<gpio_num_t>(m_pinA));
    int b = gpio_get_level(static_cast<gpio_num_t>(m_pinB));
    int state = (a << 1) | b;
    if (state == m_lastState) return;
    if (nowMs - m_lastEncoderMs < kEncoderDebounceMs) return;

    int prev = m_lastState;
    int8_t movement = kTable[prev][state];
    m_lastState = state;
    m_lastEncoderMs = nowMs;

    if (movement == 0) {
        m_stepAccum = 0;
        return;
    }

    m_stepAccum += movement;
    if (state != m_detentState) {
        if (m_stepAccum > 8 || m_stepAccum < -8) {
            m_stepAccum = 0;
        }
        return;
    }

    if (nowMs - m_lastEmitMs >= kEncoderEmitGapMs) {
        if (m_stepAccum >= 3) {
            ++m_delta;
            m_lastEmitMs = nowMs;
        } else if (m_stepAccum <= -3) {
            --m_delta;
            m_lastEmitMs = nowMs;
        }
    }
    m_stepAccum = 0;
}

} // namespace incubator::devices
