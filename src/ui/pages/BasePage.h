#pragma once
#include "ui/UiModel.h"
#include "devices/St7789Display.h"
#include "ui/UiLayout.h"
#include "ui/UiColors.h"

namespace incubator::ui::pages
{
    class BasePage
    {
    protected:
        UiModel& m_model;
        incubator::devices::St7789Display& m_display;

    public:
        BasePage(UiModel& model, incubator::devices::St7789Display& display)
            : m_model(model), m_display(display) {}
        
        virtual ~BasePage() = default;

        // 각 페이지가 반드시 구현해야 하는 렌더링 함수
        virtual void render(uint32_t nowMs) = 0;

    protected:
        // 공통 UI 유틸리티 (MainUiRenderer에서 이동)
        bool hasUtf8(const char* text);
        void drawProgressBar(int x, int y, int w, int h, uint8_t pct, uint32_t color);
        void drawPill(int x, int y, int w, const char* label, uint32_t color);
        void drawButton(int x, int y, int w, int h, const char* label, uint32_t bgColor);
        void drawRow(int y, const char* label, const char* value, bool selected = false, uint32_t valColor = 0, int valSize = 1);

        void renderConfirm(const char*, const char* line1, const char* line2);
        void drawStatusIcons(uint32_t nowMs);
        const char* presetName(uint8_t i);
    };
}