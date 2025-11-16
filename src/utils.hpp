#pragma once

#include <cstdint>

#define VK_CHECK_AND_SET(var, result, message) \
    try { \
        var = result; \
    } \
    catch (std::runtime_error &e) \
    { \
        std::print("{}\n{}", message, e.what()); \
        exit(1); \
    }

uint32_t Clamp(uint32_t value, uint32_t min, uint32_t max)
{
    if (value < min) return min;
    else if (value > max) return max;
    return max;
}

int32_t Clamp(int32_t value, int32_t min, int32_t max)
{
    if (value < min) return min;
    else if (value > max) return max;
    return max;
}
