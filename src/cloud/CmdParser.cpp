#ifdef INCUBATOR_ENABLE_CLOUD
#include "cloud/CmdParser.h"
#include "app/AppController.h"
#include <ArduinoJson.h>
#include <cstring>

namespace incubator::cloud
{

static app::Cmd commandFromString(const char* cmd)
{
    if (strcmp(cmd, "START_BATCH") == 0) return app::Cmd::StartBatch;
    if (strcmp(cmd, "STOP_BATCH") == 0) return app::Cmd::StopBatch;
    if (strcmp(cmd, "PATCH_PLAN_ROW") == 0) return app::Cmd::PatchPlanRow;
    if (strcmp(cmd, "RESET_PLAN") == 0) return app::Cmd::ResetPlan;
    if (strcmp(cmd, "UPDATE_SETTINGS") == 0) return app::Cmd::UpdateSettings;
    if (strcmp(cmd, "CLEAR_SAFE_MODE") == 0) return app::Cmd::ClearSafeMode;
    return static_cast<app::Cmd>(0);
}

bool CmdParser::parse(const char* json,
                      app::AppController& ctrl)
{
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, json);
    if (err) return false;

    const char* cmdStr = doc["cmd"];
    if (!cmdStr) return false;
    app::Cmd cmd = commandFromString(cmdStr);
    if (cmd == static_cast<app::Cmd>(0)) return false;

    if (cmd == app::Cmd::PatchPlanRow) {
        auto payload = doc["payload"];
        if (!payload.is<JsonObject>()) return false;
        domain::IncubationPlanRow row;
        row.day = payload["day"] | 0;
        row.targetTempC = payload["targetTempC"] | 0.0f;
        row.targetHumidityPct = payload["targetHumidityPct"] | 0.0f;
        row.turningEnabled = payload["turningEnabled"] | false;
        row.turningIntervalMin = payload["turningIntervalMin"] | 0;
        return ctrl.applyCommand(app::Cmd::PatchPlanRow, &row, sizeof(row));
    }

    return ctrl.applyCommand(cmd);
}

} // namespace incubator::cloud
#endif
