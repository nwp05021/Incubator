#include "ui/ProvisioningRenderer.h"
#include "ui/UiColors.h"
#include <cstdio>

namespace incubator::ui
{
namespace
{
    static constexpr uint32_t kBlack  = 0x0000U;
    static constexpr uint32_t kWhite  = 0xFFFFU;
    static constexpr uint32_t kPanel  = 0x0841U;
    static constexpr uint32_t kLine   = 0x18E3U;
    static constexpr uint32_t kAccent = 0x0679U;
    static constexpr uint32_t kOk     = 0x07E0U;
    static constexpr uint32_t kWarn   = 0xFFE0U;
    static constexpr uint32_t kDanger = 0xF800U;

    void drawText(devices::St7789Display& d, int x, int y, const char* text,
                  uint8_t size, uint32_t fg, uint32_t bg)
    {
        d.setTextSize(size);
        d.setTextColor(fg, bg);
        d.drawText(x, y, text);
    }

    void drawStatus(devices::St7789Display& d, const char* text, uint32_t color)
    {
        d.fillRect(248, 5, 62, 18, kPanel);
        d.drawRect(248, 5, 62, 18, color);
        drawText(d, 258, 9, text, 1, color, kPanel);
    }

    void formatTime(uint32_t ms, char* out, size_t len)
    {
        uint32_t sec = ms / 1000U;
        uint32_t min = sec / 60U;
        sec %= 60U;
        std::snprintf(out, len, "%02u:%02u", static_cast<unsigned>(min), static_cast<unsigned>(sec));
    }
}

void ProvisioningRenderer::render(uint32_t nowMs)
{
    (void)nowMs;

    // Same QR payload shape as Espressif's Arduino/IDF provisioning examples.
    char payload[160];
    std::snprintf(payload, sizeof(payload),
                  "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}",
                  m_model.provisioningName,
                  m_model.provisioningPop);

    m_display.fillScreen(Color::kBg);

    m_display.fillRect(0, 0, 320, 28, kBlack);
    drawText(m_display, 10, 7, "BLE WiFi Provisioning", 1, Color::kText, kBlack);

    if (m_model.provisioningSucceeded) drawStatus(m_display, "DONE", kOk);
    else if (m_model.provisioningFailed) drawStatus(m_display, "FAIL", kDanger);
    else drawStatus(m_display, "SCAN", kAccent);

    // Large scan target with a real quiet zone. 204 px is close to the largest
    // practical QR size while leaving readable pairing info on this 320x240 LCD.
    m_display.fillRect(4, 30, 216, 206, kWhite);
    m_display.drawRect(3, 29, 218, 208, kAccent);
    m_display.drawQrCode(payload, 10, 36, 204, 8, true);

    m_display.fillRect(226, 30, 90, 206, kPanel);
    m_display.drawRect(226, 30, 90, 206, kLine);
    drawText(m_display, 236, 42, "APP", 1, kAccent, kPanel);
    drawText(m_display, 236, 58, "ESP", 2, Color::kText, kPanel);
    drawText(m_display, 236, 82, "Prov", 2, Color::kText, kPanel);

    m_display.drawLine(234, 112, 308, 112, kLine);
    drawText(m_display, 236, 122, "NAME", 1, kAccent, kPanel);
    drawText(m_display, 236, 138, m_model.provisioningName, 1, Color::kText, kPanel);

    drawText(m_display, 236, 162, "PIN", 1, kAccent, kPanel);
    drawText(m_display, 236, 178, m_model.provisioningPop, 1, Color::kText, kPanel);

    char remaining[16];
    formatTime(m_model.provisioningRemainingMs, remaining, sizeof(remaining));
    drawText(m_display, 236, 204, remaining, 1,
             m_model.provisioningFailed ? kWarn : Color::kTextDim, kPanel);
    drawText(m_display, 236, 220, m_model.provisioningMessage, 1,
             m_model.provisioningFailed ? kWarn : Color::kTextDim, kPanel);
}

} // namespace incubator::ui
