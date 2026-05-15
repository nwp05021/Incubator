#pragma once
#include "domain/RuntimeState.h"
#include "domain/IncubationBatch.h"
#include <cstddef>

namespace incubator::cloud
{
    class TelemetryBuilder
    {
    public:
        static size_t build(const domain::RuntimeState& state,
                            const domain::IncubationBatch& batch,
                            const char* deviceId,
                            char* buf, size_t bufSize);

        static size_t buildHealth(const domain::RuntimeState& state,
                                  const char* deviceId,
                                  char* buf, size_t bufSize);
    };
}
