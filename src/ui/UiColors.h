// UiColors.h
#pragma once
#include <cstdint>

namespace incubator::ui::Color
{
    // --- 프리미엄 가전 아키텍처 테마 (Luxury Appliance Theme V4) ---
    static constexpr uint32_t kBg         = 0x10A3U; // 깊고 묵직한 오닉스 차콜 (메인 배경)
    static constexpr uint32_t kHeader     = 0x0841U; // 상단바 - 몰입감을 주는 완벽한 다크 솔리드
    static constexpr uint32_t kFooter     = 0x0841U; // 하단바 - 상단과 대칭을 이루는 차분한 단차감
    
    static constexpr uint32_t kText       = 0xFFFFU; // 크리스탈 화이트 (메인 텍스트)
    static constexpr uint32_t kTextDim    = 0xBDD7U; // 메탈릭 플래티넘 실버 (서브 라벨/가이드)
    
    // 온도와 습도의 시각적 인지 강도를 1:1로 맞춘 하이엔드 인디케이터 컬러
    static constexpr uint32_t kAccentTemp = 0xFBE4U; // 따뜻하고 리치한 쿠퍼 앰버 오렌지 (온도 수치)
    static constexpr uint32_t kAccentHumi = 0x07FFU; // 어두운 화면에서 가독성이 가장 뛰어난 프리미엄 네온 사이언 (습도 수치)
    
    static constexpr uint32_t kAlarmHigh  = 0xD906U; // 크림슨 로즈 레드
    static constexpr uint32_t kAlarmLow   = 0x221FU; 
    
    static constexpr uint32_t kOnIcon     = 0x3666U; // 에메랄드 민트 그린
    static constexpr uint32_t kOffIcon    = 0x2146U; // 배경 속에 정갈하게 숨겨지는 다크 슬레이트
    static constexpr uint32_t kSelected   = 0x2B54U; // [해결] 메뉴 및 포커스 선택 시 부드러운 반전 음영을 주는 딥 인디고 블루
    static constexpr uint32_t kDivider    = 0x18E5U; // 가느다랗고 모던한 그리드 분리선
}