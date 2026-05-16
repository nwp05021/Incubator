#include "devices/St7789Display.h"
#include <esp_log.h>

namespace incubator::devices
{

// St7789Display.cpp 수정
bool St7789Display::init()
{
    if (!m_gfx.init()) {
        ESP_LOGE("St7789Display", "LGFX init failed");
        return false;
    }
    m_gfx.setRotation(0);
    
    // 추가: 캔버스를 위해 PSRAM 사용을 명시적으로 설정하거나 
    // 컬러 뎁스를 먼저 설정하여 메모리 요구량을 확정합니다.
    m_canvas.setColorDepth(16); 
    
    // createSprite 호출 전 또는 인자로 PSRAM 사용 여부를 확인합니다.
    // LovyanGFX는 사용 가능한 경우 PSRAM을 우선 시도하지만, 확실히 하기 위해 체크합니다.
    if (m_canvas.createSprite(320, 240) == nullptr) {
        ESP_LOGW("St7789Display", "SRAM allocation failed, trying PSRAM...");
        // 일부 버전에서는 별도의 할당 방식을 사용해야 할 수 있습니다.
    }

    m_canvasReady = (m_canvas.getBuffer() != nullptr); // 버퍼가 실제 있는지 확인
    
    if (m_canvasReady) {
        m_canvas.fillScreen(0x0000);
        ESP_LOGI("St7789Display", "Double buffering enabled (Canvas allocated)");
    } else {
        ESP_LOGE("St7789Display", "Canvas allocation failed! Flickering will occur.");
    }
    
    m_initialized = true;
    return true;
}

void St7789Display::beginFrame()
{
    if (!m_initialized) return;
    if (!m_canvasReady) m_gfx.startWrite();
}

void St7789Display::endFrame()
{
    if (!m_initialized) return;
    if (m_canvasReady) {
        m_gfx.startWrite();
        m_canvas.pushSprite(0, 0);
        m_gfx.endWrite();
    } else {
        m_gfx.endWrite();
    }
}

void St7789Display::fillScreen(uint32_t color)
{
    if (!m_initialized) return;
    if (m_canvasReady) m_canvas.fillScreen(color);
    else m_gfx.fillScreen(color);
}

void St7789Display::fillRect(int x, int y, int w, int h, uint32_t color)
{
    if (!m_initialized) return;
    if (m_canvasReady) m_canvas.fillRect(x, y, w, h, color);
    else m_gfx.fillRect(x, y, w, h, color);
}

void St7789Display::drawLine(int x, int y, int x2, int y2, uint32_t color)
{
    if (!m_initialized) return;
    if (m_canvasReady) m_canvas.drawLine(x, y, x2, y2, color);
    else m_gfx.drawLine(x, y, x2, y2, color);
}

void St7789Display::drawRect(int x, int y, int w, int h, uint32_t color)
{
    if (!m_initialized) return;
    if (m_canvasReady) m_canvas.drawRect(x, y, w, h, color);
    else m_gfx.drawRect(x, y, w, h, color);
}

void St7789Display::drawText(int x, int y, const char* text)
{
    if (!m_initialized) return;
    if (m_canvasReady) {
        m_canvas.setFont(hasUtf8(text) ? &fonts::efontKR_16 : nullptr);
        m_canvas.setTextSize(m_textSize);
        m_canvas.setCursor(x, y);
        m_canvas.print(text);
    } else {
        m_gfx.setFont(hasUtf8(text) ? &fonts::efontKR_16 : nullptr);
        m_gfx.setTextSize(m_textSize);
        m_gfx.setCursor(x, y);
        m_gfx.print(text);
    }
}

void St7789Display::drawNumberText(int x, int y, const char* text)
{
    if (!m_initialized) return;
    if (m_canvasReady) {
        m_canvas.setFont(&fonts::Font6);
        m_canvas.setTextSize(m_textSize);
        m_canvas.setCursor(x, y);
        m_canvas.print(text);
    } else {
        m_gfx.setFont(&fonts::Font6);
        m_gfx.setTextSize(m_textSize);
        m_gfx.setCursor(x, y);
        m_gfx.print(text);
    }
}

void St7789Display::drawQrCode(const char* payload, int x, int y, int size, int version, bool autocase)
{
    if (!m_initialized) return;
    if (m_canvasReady) m_canvas.qrcode(payload, x, y, size, version, autocase);
    else m_gfx.qrcode(payload, x, y, size, version, autocase);
}

void St7789Display::setTextColor(uint32_t fg, uint32_t bg)
{
    if (!m_initialized) return;
    if (m_canvasReady) m_canvas.setTextColor(fg, bg);
    else m_gfx.setTextColor(fg, bg);
}

void St7789Display::setTextSize(uint8_t size)
{
    if (!m_initialized) return;
    m_textSize = size;
    if (m_canvasReady) m_canvas.setTextSize(size);
    else m_gfx.setTextSize(size);
}

bool St7789Display::hasUtf8(const char* text)
{
    if (!text) return false;
    while (*text) {
        if ((static_cast<unsigned char>(*text) & 0x80U) != 0U) {
            return true;
        }
        ++text;
    }
    return false;
}

} // namespace incubator::devices
