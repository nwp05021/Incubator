#pragma once
#include <cstdint>

namespace incubator::domain
{
    enum class Species : uint8_t
    {
        Chicken = 0,
        Duck    = 1,
        Quail   = 2,
        Goose   = 3,
        Custom  = 4
    };

    inline const char* speciesName(Species s)
    {
        switch (s) {
            case Species::Chicken: return "Chicken";
            case Species::Duck:    return "Duck";
            case Species::Quail:   return "Quail";
            case Species::Goose:   return "Goose";
            default:               return "Custom";
        }
    }
}
