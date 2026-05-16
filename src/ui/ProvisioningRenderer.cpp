#include "ui/ProvisioningRenderer.h"
#include "ui/UiColors.h"
#include <cstdio>
#include <cstring>

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

void ProvisioningRenderer::reset()
{
    m_fullRendered = false;
    m_lastSucceeded = false;
    m_lastFailed = false;
    m_lastRemainingSec = UINT32_MAX;
    m_lastPayload[0] = '\0';
    m_lastMessage[0] = '\0';
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

    const bool payloadChanged = std::strcmp(payload, m_lastPayload) != 0;
    const bool fullRender = !m_fullRendered || payloadChanged;
    const uint32_t remainingSec = m_model.provisioningRemainingMs / 1000U;
    const bool statusChanged = fullRender ||
                               m_model.provisioningSucceeded != m_lastSucceeded ||
                               m_model.provisioningFailed != m_lastFailed;
    const bool detailsChanged = fullRender ||
                                remainingSec != m_lastRemainingSec ||
                                std::strcmp(m_model.provisioningMessage, m_lastMessage) != 0;

    if (fullRender) {
        m_display.fillScreen(Color::kBg);

        m_display.fillRect(0, 0, 320, 28, kBlack);
        drawText(m_display, 10, 7, "BLE WiFi Provisioning", 1, Color::kText, kBlack);

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
    }

    if (statusChanged) {
        if (m_model.provisioningSucceeded) drawStatus(m_display, "DONE", kOk);
        else if (m_model.provisioningFailed) drawStatus(m_display, "FAIL", kDanger);
        else drawStatus(m_display, "SCAN", kAccent);
    }

    if (detailsChanged) {
        char remaining[16];
        formatTime(m_model.provisioningRemainingMs, remaining, sizeof(remaining));
        m_display.fillRect(236, 202, 72, 32, kPanel);
        drawText(m_display, 236, 204, remaining, 1,
                 m_model.provisioningFailed ? kWarn : Color::kTextDim, kPanel);
        drawText(m_display, 236, 220, m_model.provisioningMessage, 1,
                 m_model.provisioningFailed ? kWarn : Color::kTextDim, kPanel);
    }

    std::strncpy(m_lastPayload, payload, sizeof(m_lastPayload) - 1U);
    m_lastPayload[sizeof(m_lastPayload) - 1U] = '\0';
    std::strncpy(m_lastMessage, m_model.provisioningMessage, sizeof(m_lastMessage) - 1U);
    m_lastMessage[sizeof(m_lastMessage) - 1U] = '\0';
    m_lastSucceeded = m_model.provisioningSucceeded;
    m_lastFailed = m_model.provisioningFailed;
    m_lastRemainingSec = remainingSec;
    m_fullRendered = true;
}

} // namespace incubator::ui
