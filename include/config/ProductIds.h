#pragma once
#include <cstdint>

namespace incubator::config
{
    static constexpr uint16_t Sensor_Temp      = 1001U;
    static constexpr uint16_t Sensor_Humidity  = 1002U;

    static constexpr uint16_t Relay_Heater     = 2001U;
    static constexpr uint16_t Relay_Humidifier = 2002U;
    static constexpr uint16_t Relay_Turner     = 2003U;
    static constexpr uint16_t Output_Buzzer    = 2004U;
    static constexpr uint16_t Output_Fan       = 2005U;
}
