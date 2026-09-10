#pragma once

#include <DirectXMath.h>

namespace Murder
{
    struct MaterialGpuData
    {
        DirectX::XMFLOAT3 ambient{ 1.0f, 1.0f, 1.0f };
        float             shininess = 32.0f;

        DirectX::XMFLOAT3 diffuse{ 1.0f, 1.0f, 1.0f };
        float             alpha = 1.0f;

        DirectX::XMFLOAT3 specular{ 0.0f, 0.0f, 0.0f };
        float             reflect = 0.0f;
    };
} // namespace Murder
