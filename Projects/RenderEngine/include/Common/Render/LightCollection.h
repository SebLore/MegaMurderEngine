#pragma once

#include <cstdint>

#include <Core/MurderCore.hpp>

namespace Murder
{
    struct alignas(16) PointLightGpuData
    {
        Vector3 position = { 0.0f, 0.0f, 0.0f };
        float   range    = 10.0f;

        Vector3 color     = { 1.0f, 1.0f, 1.0f };
        float   intensity = 1.0f;

        Vector3 attenuation = { 1.0f, 0.0f, 0.01f };
        float   pad0        = 0.0f;
    };

    struct alignas(16) DirectionalLightGpuData
    {
        //DirectX::XMFLOAT4X4 vp{}; // for shadow mapping
        Vector3 direction = { 0.0f, -1.0f, 0.0f };
        float   intensity = 1.0f;

        Vector3 color = { 1.0f, 1.0f, 1.0f };
        float   pad0  = 0.0f;
    };

    struct alignas(16) SpotLightGpuData
    {
        //DirectX::XMFLOAT4X4 vp{}; // for shadow mapping

        Vector3 position = { 0.0f, 0.0f, 0.0f };
        float   range    = 20.0f;

        Vector3 direction = { 0.0f, -1.0f, 0.0f };
        float   intensity = 1.0f;

        Vector3 color    = { 1.0f, 1.0f, 1.0f };
        float   innerCos = 0.95f;

        Vector3 attenuation = { 1.0f, 0.0f, 0.01f };
        float   outerCos    = 0.85f;
    };

    struct alignas(16) LightCollectionBufferData
    {
        uint32_t pointCount       = 0;
        uint32_t directionalCount = 0;
        uint32_t spotCount        = 0;

        float useShaders = 0.0f;

        Vector3 ambient = { 0.1f, 0.1f, 0.1f };
        float   pad1    = 0.0f;
    };

    // we don't want to go over this number of lights
    static constexpr uint32_t MAX_POINT_LIGHTS_GPU       = 32;
    static constexpr uint32_t MAX_DIRECTIONAL_LIGHTS_GPU = 16;
    static constexpr uint32_t MAX_SPOT_LIGHTS_GPU        = 16;
} // namespace Murder
