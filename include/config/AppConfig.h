#pragma once

// ── 빌드 타임 상수 ────────────────────────────────────────────────
#define INCUBATOR_FW_VERSION        "1.0.0"
#define INCUBATOR_DEVICE_ID_PREFIX  "INC-"

// ── Watchdog ──────────────────────────────────────────────────────
static constexpr uint32_t kWatchdogTimeoutMs = 5000U;

// ── 주기 상수 (모든 모듈이 참조) ──────────────────────────────────
static constexpr uint32_t kSensorPollMs     = 2000U;   // AHT20 측정 주기
static constexpr uint32_t kControlTickMs    =  500U;   // 제어 루프 주기
static constexpr uint32_t kSchedulerTickMs  = 10000U;  // Day 계산 주기
static constexpr uint32_t kUiTickMs         =  100U;   // UI 갱신 주기
static constexpr uint32_t kTelemetryMs      = 60000U;  // Cloud 발행 주기 (Phase 2)
static constexpr uint32_t kHealthMs         = 30000U;  // AWS IoT health heartbeat interval

// ── Phase 조건부 컴파일 ──────────────────────────────────────────
// platformio.ini build_flags 로 활성화
// #define INCUBATOR_ENABLE_CLOUD
// #define INCUBATOR_ENABLE_PROVISIONING

#ifdef INCUBATOR_ENABLE_CLOUD
#define AWS_IOT_ENDPOINT   "a3qvzpdja8zc70-ats.iot.ap-northeast-2.amazonaws.com"
#define INCUBATOR_DEVICE_ID "Incubator-1001"
#endif
