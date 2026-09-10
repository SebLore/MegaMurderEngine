#pragma once

#include <Core/Math/Transform.h>
#include <DirectXMath.h>

namespace Murder
{
    // gpu-data struct uploaded to a constant buffer every frame per-model
    // TODO: use InstanceTransform for instanced drawing
    struct alignas(16) TransformGpuData
    {
        DirectX::XMFLOAT4X4 world{};
        DirectX::XMFLOAT4X4 normal{}; // inv transpose of world, read as 3x3
    };
    static_assert(sizeof(TransformGpuData) % 16 == 0, "TransformGpuData must be 16-byte aligned in size.");

    /*
     * Instancing/batching support below
     * We implement this if we have the time ,for now default to just a constant buffer
     */

    // instancing requires transforms to be set up this way to be efficiently uploaded as a structured buffer, and read as 3x4 in the shader
    struct alignas(16) InstanceTransform3x4Gpu
    {
        DirectX::XMFLOAT4 row0{};
        DirectX::XMFLOAT4 row1{};
        DirectX::XMFLOAT4 row2{};
    };
    static_assert(sizeof(InstanceTransform3x4Gpu) == 48, "InstanceTransform3x4Gpu should be 48 bytes.");

    namespace TransformGpu
    {
        [[nodiscard]] TransformGpuData        ToGpuData(const Transform& t);
        [[nodiscard]] InstanceTransform3x4Gpu ToInstance3x4(const Transform& t);
    } // namespace TransformGpu
} // namespace Murder
