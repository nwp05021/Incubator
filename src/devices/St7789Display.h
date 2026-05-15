#pragma once
#include "LgfxConfig.h"
#include <cstdint>

namespace incubator::devices
{
    class St7789Display
    {
    public:
        bool init();
        void beginFrame();
        void endFrame();
        void fillScreen(uint32_t color);
        void fillRect(int x, int y, int w, int h, uint32_t color);
        void drawLine(int x, int y, int x2, int y2, uint32_t color);
        void drawRect(int x, int y, int w, int h, uint32_t color);
        void drawText(int x, int y, const char* text);
        void drawNumberText(int x, int y, const char* text);
        void drawQrCode(const char* payload, int x, int y, int size, int version = 6, bool autocase = true);
        void setTextColor(uint32_t fg, uint32_t bg = 0x0000U);
        void setTextSize(uint8_t size);
        LGFX& gfx() { return m_gfx; }

    private:
        LGFX m_gfx;
        LGFX_Sprite m_canvas{&m_gfx};
        bool m_initialized = false;
        bool m_canvasReady = false;
        uint8_t m_textSize = 1;

        static bool hasUtf8(const char* text);
    };
}
