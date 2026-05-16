#include "devices/I2cBus.h"
#include <esp_log.h>
#include <driver/gpio.h>

static const char* TAG = "I2cBus";

namespace incubator::devices
{

I2cBus::~I2cBus()
{
    for (auto& pair : m_devices) {
        i2c_master_bus_rm_device(pair.second);
    }
    if (m_busHandle) {
        i2c_del_master_bus(m_busHandle);
    }
}

bool I2cBus::init(int sdaPin, int sclPin, uint32_t freqHz)
{
    if (m_initialized) return true;

    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = static_cast<gpio_num_t>(sdaPin);
    bus_cfg.scl_io_num = static_cast<gpio_num_t>(sclPin);
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &m_busHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(err));
        return false;
    }

    m_initialized = true;
    return true;
}

i2c_master_dev_handle_t I2cBus::getOrCreateDevice(uint8_t addr)
{
    if (m_devices.count(addr)) return m_devices[addr];

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = addr;
    dev_cfg.scl_speed_hz = 400000;

    i2c_master_dev_handle_t dev_handle;
    if (i2c_master_bus_add_device(m_busHandle, &dev_cfg, &dev_handle) == ESP_OK) {
        m_devices[addr] = dev_handle;
        return dev_handle;
    }
    return nullptr;
}

bool I2cBus::write(uint8_t addr, const uint8_t* data, size_t len)
{
    auto dev = getOrCreateDevice(addr);
    if (!dev) return false;
    return i2c_master_transmit(dev, data, len, kTimeoutMs) == ESP_OK;
}

bool I2cBus::read(uint8_t addr, uint8_t* buf, size_t len)
{
    auto dev = getOrCreateDevice(addr);
    if (!dev) return false;
    return i2c_master_receive(dev, buf, len, kTimeoutMs) == ESP_OK;
}

bool I2cBus::writeRead(uint8_t addr, const uint8_t* txData, size_t txLen, uint8_t* rxBuf, size_t rxLen)
{
    auto dev = getOrCreateDevice(addr);
    if (!dev) return false;
    return i2c_master_transmit_receive(dev, txData, txLen, rxBuf, rxLen, kTimeoutMs) == ESP_OK;
}

bool I2cBus::isReady(uint8_t addr)
{
    if (!m_initialized) return false;
    return i2c_master_probe(m_busHandle, addr, kTimeoutMs) == ESP_OK;
}

} // namespace incubator::devices