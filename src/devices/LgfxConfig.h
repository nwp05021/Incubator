#pragma once
#include <LovyanGFX.hpp>
#include "config/PinConfig.h"

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789  _panel;
    lgfx::Bus_SPI       _bus;

public:
    LGFX()
    {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.freq_write = 40000000;
            cfg.spi_mode = 0;
            cfg.pin_sclk = incubator::config::Pin::TFT_SCLK;
            cfg.pin_mosi = incubator::config::Pin::TFT_MOSI;
            cfg.pin_miso = -1;
            cfg.pin_dc = incubator::config::Pin::TFT_DC;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs     = incubator::config::Pin::TFT_CS;
            cfg.pin_rst    = incubator::config::Pin::TFT_RST;
            cfg.pin_busy   = -1;
            cfg.panel_width  = 240;
            cfg.panel_height = 320;
            cfg.offset_rotation = 1;
            cfg.invert = true;
            _panel.config(cfg);
        }
        setPanel(&_panel);
    }
};
