#pragma once
#include "app/AppController.h"

namespace incubator::cloud
{
    class CmdParser
    {
    public:
        static bool parse(const char* json,
                          app::AppController& ctrl);
    };
}
