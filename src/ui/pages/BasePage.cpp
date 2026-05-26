#include "ui/pages/BasePage.h"
#include <cstring>

namespace incubator::ui::pages
{
    const char* BasePage::presetName(uint8_t i)
    {
        switch (i) {
            case 0: return "닭";
            case 1: return "오리";
            case 2: return "메추리";
            case 3: return "거위";
            default: return "닭";
        }
    }

    bool BasePage::hasUtf8(const char* text)
    {
        if (!text) return false;
        while (*text) {
            if ((static_cast<unsigned char>(*text) & 0x80U) != 0U) return true;
            ++text;
        }
        return false;
    }

    void BasePage::drawProgressBar(int x, int y, int w, int h, uint8_t pct, uint32_t color)
    {
        if (pct > 100U) pct = 100U;
        int fillW = (w - 4) * static_cast<int>(pct) / 100;
        m_display.drawRect(x, y, w, h, Color::kPanelSoft); // 기존 kPanelSoft 대체
        m_display.fillRect(x + 2, y + 2, w - 4, h - 4, 0x0000U);
        if (fillW > 0) m_display.fillRect(x + 2, y + 2, fillW, h - 4, color);
    }

    void BasePage::drawPill(int x, int y, int w, const char* label, uint32_t color)
    {
        m_display.drawRect(x, y + 1, w, 24, color);
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(x + (hasUtf8(label) ? 8 : 6), y + 4, label);
    }

    void BasePage::drawButton(int x, int y, int w, int h, const char* label, uint32_t bgColor)
    {
        m_display.fillRect(x, y, w, h, bgColor);
        m_display.drawRect(x, y, w, h, 0x31CDU); // kPanelSoft
        m_display.setTextSize(1);
        m_display.drawText(x + (w / 2) - (std::strlen(label) * 3), y + (h / 2) - 4, label);
    }

    void BasePage::drawRow(int y, const char* label, const char* value, bool selected, uint32_t valColor, int valSize)
    {
        uint32_t bg     = selected ? Color::kText : 0x2146U; // kPanel
        uint32_t border = selected ? Color::kText : 0x31CDU; // kPanelSoft
        uint32_t textFg = selected ? Color::kBg   : Color::kText; 

        m_display.fillRect(12, y, 296, 27, bg);
        m_display.drawRect(12, y, 296, 27, border);

        m_display.setTextSize(1);
        m_display.setTextColor(textFg, bg);
        m_display.drawText(20, y + 7, label); 

        uint32_t vFg = selected ? Color::kBg : valColor;
        m_display.setTextColor(vFg, bg);
        
        m_display.setTextSize(valSize);
        if (valSize == 2) {
            m_display.drawText(270 - (std::strlen(value) * 12), y + 6, value);
        } else {
            m_display.drawText(270 - (std::strlen(value) * 6), y + 7, value);
        }
    }

    void BasePage::renderConfirm(const char*, const char* line1, const char* line2)
    {
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(34, 82, line1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(34, 104, line2);
        drawRow(148, "아니오", "취소", m_model.confirmCursor == 0, Color::kOffIcon);
        drawRow(180, "예", "실행", m_model.confirmCursor == 1, Color::kDanger);
    }

    void BasePage::drawStatusIcons(uint32_t nowMs)
    {
        m_display.setTextSize(1);
        const bool blinkVisible = ((nowMs / 500U) % 2U) == 0U;

        auto drawFooterPill = [&](int x, int w, const char* label, bool active) {
            uint32_t border = Color::kOffIcon;
            uint32_t fill   = Color::kFooter; 
            uint32_t text   = Color::kTextDim; 

            if (active) {
                border = Color::kOnIcon; 
                if (blinkVisible) {
                    fill = Color::kOnIcon;
                    text = Color::kText; 
                } else {
                    fill = Color::kFooter;
                    text = Color::kOnIcon; 
                }
            }

            m_display.fillRect(x, 218, w, 20, fill);
            m_display.drawRect(x, 218, w, 20, border);
            m_display.setTextColor(text, fill);
            m_display.drawText(x + (hasUtf8(label) ? 8 : 6), 222, label);
        };

        drawFooterPill(8,  42, "히터", m_model.heaterOn);
        drawFooterPill(54, 42, "가습", m_model.humidifierOn);
        drawFooterPill(100, 42, "전란", m_model.turnerOn);
        drawFooterPill(146, 32, "팬",  m_model.fanOn);
    }
}