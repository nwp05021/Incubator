#ifdef INCUBATOR_ENABLE_CLOUD
#include "cloud/TelemetryBuilder.h"
#include <cstdio>
#include <ctime>

namespace incubator::cloud
{

size_t TelemetryBuilder::build(const domain::RuntimeState& state,
                               const domain::IncubationBatch& batch,
                               const char* deviceId,
                               char* buf, size_t bufSize)
{
    if (!buf || bufSize == 0U) return 0U;

    const char* batchId = batch.batchId[0] ? batch.batchId : "";
    int written = std::snprintf(
        buf, bufSize,
        "{\"deviceId\":\"%s\","
        "\"batchId\":\"%s\","
        "\"ts\":%u,"
        "\"uptimeMs\":%u,"
        "\"day\":%u,"
        "\"sensor\":{\"tempC\":%.2f,\"humidityPct\":%.2f,"
        "\"tempOk\":%s,\"humiOk\":%s,"
        "\"tempWarning\":%s,\"humiWarning\":%s},"
        "\"actuator\":{\"heater\":%s,\"humidifier\":%s,\"turner\":%s,\"fan\":%s},"
        "\"target\":{\"tempC\":%.2f,\"humidityPct\":%.2f},"
        "\"alarm\":{\"temp\":%s,\"humi\":%s,\"safeMode\":%s},"
        "\"progress\":{\"currentDay\":%u,\"totalDays\":%u,\"lockdown\":%s}}",
        deviceId ? deviceId : "",
        batchId,
        static_cast<unsigned>(std::time(nullptr)),
        static_cast<unsigned>(state.uptimeMs),
        static_cast<unsigned>(state.currentDay),
        static_cast<double>(state.currentTempC),
        static_cast<double>(state.currentHumidityPct),
        state.tempSensorOk ? "true" : "false",
        state.humiSensorOk ? "true" : "false",
        state.tempSensorWarning ? "true" : "false",
        state.humiSensorWarning ? "true" : "false",
        state.heaterOn ? "true" : "false",
        state.humidifierOn ? "true" : "false",
        state.turnerOn ? "true" : "false",
        state.fanOn ? "true" : "false",
        static_cast<double>(state.targetTempC),
        static_cast<double>(state.targetHumidityPct),
        state.tempAlarmActive ? "true" : "false",
        state.humiAlarmActive ? "true" : "false",
        state.safeMode ? "true" : "false",
        static_cast<unsigned>(state.currentDay),
        static_cast<unsigned>(state.totalDays),
        state.lockdownActive ? "true" : "false");

    if (written < 0) return 0U;
    if (static_cast<size_t>(written) >= bufSize) {
        buf[bufSize - 1U] = '\0';
        return bufSize - 1U;
    }
    return static_cast<size_t>(written);
}

size_t TelemetryBuilder::buildHealth(const domain::RuntimeState& state,
                                     const char* deviceId,
                                     char* buf, size_t bufSize)
{
    if (!buf || bufSize == 0U) return 0U;

    int written = std::snprintf(
        buf, bufSize,
        "{\"deviceId\":\"%s\","
        "\"status\":\"online\","
        "\"ts\":%u,"
        "\"uptimeMs\":%u,"
        "\"bootCount\":%u,"
        "\"safeMode\":%s,"
        "\"manualMode\":%s,"
        "\"sensorsOk\":%s,"
        "\"alarmsActive\":%s}",
        deviceId ? deviceId : "",
        static_cast<unsigned>(std::time(nullptr)),
        static_cast<unsigned>(state.uptimeMs),
        static_cast<unsigned>(state.bootCount),
        state.safeMode ? "true" : "false",
        state.manualMode ? "true" : "false",
        (state.tempSensorOk && state.humiSensorOk) ? "true" : "false",
        (state.tempAlarmActive || state.humiAlarmActive) ? "true" : "false");

    if (written < 0) return 0U;
    if (static_cast<size_t>(written) >= bufSize) {
        buf[bufSize - 1U] = '\0';
        return bufSize - 1U;
    }
    return static_cast<size_t>(written);
}

} // namespace incubator::cloud
#endif
