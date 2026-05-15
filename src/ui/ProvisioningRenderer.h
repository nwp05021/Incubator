#pragma once

#include "ui/UiModel.h"
#include "devices/St7789Display.h"
#include <cstdint>

namespace incubator::ui
{
    class ProvisioningRenderer
    {
    public:
        ProvisioningRenderer(UiModel& model, incubator::devices::St7789Display& display)
            : m_model(model), m_display(display) {}

        void render(uint32_t nowMs);

    private:
        UiModel& m_model;
        incubator::devices::St7789Display& m_display;
    };
}
