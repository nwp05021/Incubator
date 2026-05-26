#pragma once
#include "ui/pages/BasePage.h"

namespace incubator::ui::pages
{
    class PageAwsTest : public BasePage
    {
    public:
        using BasePage::BasePage; // 부모 생성자 상속

        void render(uint32_t nowMs) override;

    private:
        uint32_t m_txCount = 0;          // 테스트 패킷 전송 카운트
        bool m_lastTxSuccess = false;    // 마지막 전송 성공 여부
    };
}