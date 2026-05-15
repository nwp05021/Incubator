#include "devices/Aht20Driver.h"
#include <Arduino.h>
#include <esp_log.h>

static const char* TAG = "Aht20Driver";

namespace incubator::devices
{

bool Aht20Driver::init()
{
    m_ok = false;
    if (!m_bus.isInitialized()) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return false;
    }

    uint8_t cmd[3] = { 0xBE, 0x08, 0x00 };
    if (!m_bus.write(kAddress, cmd, sizeof(cmd))) {
        ESP_LOGE(TAG, "AHT20 init command failed");
        return false;
    }
    delay(10);

    cmd[0] = 0xAC;
    cmd[1] = 0x33;
    cmd[2] = 0x00;
    if (!m_bus.write(kAddress, cmd, sizeof(cmd))) {
        ESP_LOGE(TAG, "AHT20 measurement trigger failed");
        return false;
    }

    m_ok = true;
    m_measurePending = true;
    m_triggerMs = millis();
    return true;
}

bool Aht20Driver::triggerMeasurement()
{
    if (!m_bus.isInitialized()) return false;
    uint8_t cmd[3] = { 0xAC, 0x33, 0x00 };
    if (!m_bus.write(kAddress, cmd, sizeof(cmd))) {
        m_measurePending = false;
        return false;
    }
    m_measurePending = true;
    m_triggerMs = millis();
    return true;
}

bool Aht20Driver::fetchResult()
{
    if (!m_measurePending) return false;
    if ((uint32_t)(millis() - m_triggerMs) < kMeasureDelayMs) {
        return false;
    }

    uint8_t buf[6] = {};
    if (!m_bus.read(kAddress, buf, sizeof(buf))) {
        m_cached.valid = false;
        return false;
    }

    if (buf[0] & 0x80) {
        return false;
    }

    uint32_t rawHumi = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | ((uint32_t)(buf[3] >> 4));
    uint32_t rawTemp = (((uint32_t)(buf[3] & 0x0F)) << 16) | ((uint32_t)buf[4] << 8) | buf[5];
    m_cached.humidityPct = (float)rawHumi / 1048576.0f * 100.0f;
    m_cached.tempC = (float)rawTemp / 1048576.0f * 200.0f - 50.0f;
    m_cached.valid = true;
    m_cached.timestampMs = millis();
    m_measurePending = false;
    return true;
}

bool Aht20Driver::reinitialize()
{
    return init();
}

} // namespace incubator::devices
