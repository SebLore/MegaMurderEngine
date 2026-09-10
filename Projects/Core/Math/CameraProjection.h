#pragma once

#include <variant>

namespace Murder
{
    // perspective camera projection parameters
    struct PerspectiveProjection
    {
        float fovY        = 1.0471975f; // 60 degrees in radians, pi/3
        float aspectRatio = 16.0f / 9.0f;
        float nearZ       = 0.1f;
        float farZ        = 1000.0f;
    };

    // orthographic camera projection parameters
    struct OrthographicProjection
    {
        float width  = 10.0f;
        float height = 10.0f;
        float nearZ  = 0.1f;
        float farZ   = 1000.0f;
    };

    // orthographic off-center camera projection parameters
    struct OrthographicOffCenterProjection
    {
        float left   = -5.0f;
        float right  = 5.0f;
        float top    = 5.0f;
        float bottom = -5.0f;
        float nearZ  = 0.1f;
        float farZ   = 1000.0f;
    };

    // clang-format off
    /// Alias to allow CpuCamera to have one of the three types of projection
    using CameraProjection = std::variant<PerspectiveProjection, OrthographicProjection, OrthographicOffCenterProjection>;
    // clang-format on
} // namespace Murder
