#pragma once
#include "ui/UiLayout.h"
#include "UiModel.h"
#include "ui/pages/BasePage.h"
#include "ui/ProvisioningRenderer.h"
#include "devices/St7789Display.h"
#include <cstdint>
#include "ui/UiColors.h"

#include "ui/pages/PageMenu.h"
#include "ui/pages/PageManual.h"
#include "ui/pages/PageMain.h"
#include "ui/pages/PageStartDate.h"
#include "ui/pages/PageHelp.h"
#include "ui/pages/PagePreset.h"
#include "ui/pages/PagePlanList.h"
#include "ui/pages/PagePlanEdit.h"
#include "ui/pages/PageFactoryReset.h"
#include "ui/pages/PageAwsTest.h"

namespace incubator::ui
{
    class MainUiRenderer : public pages::BasePage
    {
    public:
        MainUiRenderer(UiModel& model, incubator::devices::St7789Display& display)
            : BasePage(model, display), 
            m_model(model), m_display(display), 
            m_provisioningRenderer(model, display), 
            m_pageMenu(model, display), 
            m_pageMain(model, display), 
            m_pageManual(model, display),
            m_pageStartDate(model, display),
            m_pageHelp(model, display),
            m_pagePreset(model, display),
            m_pagePlanList(model, display),
            m_pagePlanEdit(model, display),
            m_pageFactoryReset(model, display),
            m_pageAwsTest(model, display)
            {}

        void render(uint32_t nowMs);

    private:
        UiModel& m_model;
        incubator::devices::St7789Display& m_display;
        ProvisioningRenderer m_provisioningRenderer;

        pages::PageMenu m_pageMenu;
        pages::PageMain m_pageMain;
        pages::PageManual m_pageManual;
        pages::PageStartDate m_pageStartDate;
        pages::PageHelp m_pageHelp;
        pages::PagePreset m_pagePreset;
        pages::PagePlanList m_pagePlanList;
        pages::PagePlanEdit m_pagePlanEdit;
        pages::PageFactoryReset m_pageFactoryReset;
        pages::PageAwsTest m_pageAwsTest;

        uint32_t m_lastRenderMs = 0;
        uint32_t m_renderNowMs = 0;
        bool     m_hasRendered = false;
        bool     m_wasProvisioning = false;

        uint32_t m_lastHeaderHash = 0;
        uint32_t m_lastPageHash   = 0;
        uint32_t m_lastFooterHash = 0;
        bool     m_firstRender    = true;

        void renderPage1();
        void renderPage3();
        void renderConfirm(const char* title, const char* line1, const char* line2);
        void renderFactoryReset();
        void renderHeader(uint32_t nowMs);
        void renderFooter(uint32_t nowMs);
        void drawStatusBar(uint32_t nowMs);
        void drawSignalBars(int x, int y, bool connected, bool configured);
        void drawProgressBar(int x, int y, int w, int h, uint8_t pct, uint32_t color);
        void drawPill(int x, int y, int w, const char* label, uint32_t color);
        void drawButton(int x, int y, int w, int h, const char* label, uint32_t bgColor);
        // [확장] 다양한 데이터 표현의 균형을 위해 우측 밸류 폰트 크기(valSize) 인자를 추가했습니다.
        void drawRow(int y, const char* label, const char* value, bool selected = false, uint32_t valColor = 0, int valSize = 1);
    };
}