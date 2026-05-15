#pragma once
#include <cstdint>

namespace incubator::config
{
    struct Pin
    {
        // ── I2C (AHT20) ──────────────────────────────
        static constexpr int I2C_SDA       = 8;
        static constexpr int I2C_SCL       = 18;

        // ── SPI (ST7789 TFT) ─────────────────────────
        static constexpr int TFT_CS        = 14;
        static constexpr int TFT_DC        = 9;
        static constexpr int TFT_RST       = 10;
        static constexpr int TFT_SCLK      = 12;
        static constexpr int TFT_MOSI      = 11;

        // ── 디지털 출력 ──────────────────────────────
        static constexpr int SSR_HEATER     = 4;   // HIGH=ON
        static constexpr int SSR_HUMIDIFIER = 5;   // HIGH=ON
        static constexpr int RELAY_TURNER   = 6;   // HIGH=ON
        // GPIO32 caused early WDT resets on the current ESP32-S3 module.
        // Keep disabled until the actual buzzer pin is verified.
        static constexpr int BUZZER         = -1;   // HIGH=ON (Active Buzzer)

        // ── PWM 팬 ───────────────────────────────────
        // FAN PWM: GPIO18 is output-capable and avoids current I2C/SPI/EC11/relay pins,
        // boot strapping pins, USB pins, flash/PSRAM pins, and default JTAG pins.
        static constexpr int FAN_PWM        = 16;
        static constexpr int FAN_PWM_CH     = 0;    // LEDC 채널 0

        // ── EC11 로터리 엔코더 ────────────────────────
        static constexpr int ENC_A          = 1;   // CLK — ISR
        static constexpr int ENC_B          = 2;   // DT  — ISR
        static constexpr int ENC_BTN        = 3;   // SW  — 내부 풀업, Active LOW
    };
}
