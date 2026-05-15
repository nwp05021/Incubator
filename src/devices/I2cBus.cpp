#include "devices/I2cBus.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <cstring>

static const char* TAG = "I2cBus";

namespace incubator::devices
{

bool I2cBus::init(int sdaPin, int sclPin, uint32_t freqHz)
{
    if (m_initialized) {
        return true;
    }

    i2c_config_t cfg = {};
    cfg.mode = I2C_MODE_MASTER;
    cfg.sda_io_num = static_cast<gpio_num_t>(sdaPin);
    cfg.scl_io_num = static_cast<gpio_num_t>(sclPin);
    cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
    cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
    cfg.master.clk_speed = freqHz;

    esp_err_t err = i2c_param_config(m_port, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return false;
    }

    err = i2c_driver_install(m_port, cfg.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return false;
    }

    m_initialized = true;
    return true;
}

static uint32_t msToTicks(uint32_t ms)
{
    return pdMS_TO_TICKS(ms);
}

bool I2cBus::isReady(uint8_t addr)
{
    if (!m_initialized) return false;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, static_cast<uint8_t>((addr << 1) | I2C_MASTER_WRITE), true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(m_port, cmd, msToTicks(kTimeoutMs));
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

bool I2cBus::write(uint8_t addr, const uint8_t* data, size_t len)
{
    if (!m_initialized) return false;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, static_cast<uint8_t>((addr << 1) | I2C_MASTER_WRITE), true);
    if (len > 0) {
        i2c_master_write(cmd, data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(m_port, cmd, msToTicks(kTimeoutMs));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool I2cBus::read(uint8_t addr, uint8_t* buf, size_t len)
{
    if (!m_initialized) return false;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, static_cast<uint8_t>((addr << 1) | I2C_MASTER_READ), true);
    if (len > 1) {
        i2c_master_read(cmd, buf, len - 1, I2C_MASTER_ACK);
    }
    if (len > 0) {
        i2c_master_read_byte(cmd, buf + len - 1, I2C_MASTER_NACK);
    }
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(m_port, cmd, msToTicks(kTimeoutMs));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool I2cBus::writeRead(uint8_t addr,
                       const uint8_t* txData, size_t txLen,
                       uint8_t*       rxBuf,  size_t rxLen)
{
    if (!m_initialized) return false;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, static_cast<uint8_t>((addr << 1) | I2C_MASTER_WRITE), true);
    if (txLen > 0) {
        i2c_master_write(cmd, txData, txLen, true);
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, static_cast<uint8_t>((addr << 1) | I2C_MASTER_READ), true);
    if (rxLen > 1) {
        i2c_master_read(cmd, rxBuf, rxLen - 1, I2C_MASTER_ACK);
    }
    if (rxLen > 0) {
        i2c_master_read_byte(cmd, rxBuf + rxLen - 1, I2C_MASTER_NACK);
    }
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(m_port, cmd, msToTicks(kTimeoutMs));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C writeRead failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

} // namespace incubator::devices
