#pragma once
#include "UiModel.h"
#include "ui/ProvisioningRenderer.h"
#include "devices/St7789Display.h"
#include <cstdint>

namespace incubator::ui
{
    class MainUiRenderer
    {
    public:
        MainUiRenderer(UiModel& model, incubator::devices::St7789Display& display)
            : m_model(model), m_display(display), m_provisioningRenderer(model, display) {}

        void render(uint32_t nowMs);

    private:
        UiModel& m_model;
        incubator::devices::St7789Display& m_display;
        ProvisioningRenderer m_provisioningRenderer;
        uint32_t m_lastRenderMs = 0;
        uint32_t m_lastVisibleHash = 0;
        uint32_t m_renderNowMs = 0;
        bool     m_hasRendered = false;

        void renderPage0();
        void renderPage1();
        void renderPage2();
        void renderPage3();
        void renderPage4();
        void renderMenu();
        void renderStartDate();
        void renderPreset();
        void renderPlanList();
        void renderPlanEdit();
        void renderManual();
        void renderConfirm(const char* title, const char* line1, const char* line2);
        void renderFactoryReset();
        void renderHelp();
        void renderHeader();
        void renderFooter();
        void drawStatusIcons();
        void drawStatusBar();
        void drawSignalBars(int x, int y, bool connected, bool configured);
        void drawProgressBar(int x, int y, int w, int h, uint8_t pct, uint32_t color);
        void drawPill(int x, int y, int w, const char* label, uint32_t color);
        void drawRow(int y, const char* label, const char* value, bool selected = false, uint32_t accent = 0);
    };
}
