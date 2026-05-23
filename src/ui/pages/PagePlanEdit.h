#pragma once
#include "ui/pages/BasePage.h"

namespace incubator::ui::pages
{
    class PagePlanEdit : public BasePage
    {
    public:
        using BasePage::BasePage; // 부모 생성자 상속
        void render(uint32_t nowMs) override;
    };
}