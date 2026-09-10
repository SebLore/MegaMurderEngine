#pragma once

#include <DirectXMath.h>

namespace Murder
{
    struct ShadowGpuData
    {
        DirectX::XMFLOAT4X4 lightViewProj{};
        float               shadowBias = 0.00035f;
        float               shadowStrength = 0.6f;
        bool                enabled = true;
        float               pad0 = 0.0f;
        DirectX::XMFLOAT2   shadowTexelSize = { 1.0f / 2048.0f, 1.0f / 2048.0f };
        DirectX::XMFLOAT2   pad1 = { 0.0f, 0.0f };
    };
} // namespace Murder
