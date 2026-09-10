#pragma once

#include <cmath>

namespace Murder
{
    inline constexpr float PI      = 3.141592653589793f;
    inline constexpr float PI_DIV2 = PI * 0.5f;
    inline constexpr float PI_DIV4 = PI * 0.25f;

    inline constexpr float EPS     = 1e-6f; // epsilon for float comparisons
    inline constexpr float BIG_EPS = 1e-4f; // larger epsilon for things like angle comparisons

    inline bool FComp(float a, float b) { return std::abs(a - b) < EPS; }

    constexpr float ToRadians(float degs) noexcept { return degs * (PI / 180.0f); }
    constexpr float ToDegrees(float rads) noexcept { return rads * (180.0f / PI); }
} // namespace Murder
