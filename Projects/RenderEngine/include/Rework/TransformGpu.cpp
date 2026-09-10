#include "pch.h"

#include "Common/Render/TransformGpu.h"
#include <Core/Math/TransformMath.h>

namespace Murder::TransformGpu
{
    namespace
    {
        /// Stores the first three rows of a matrix into XMFLOAT4 structures for compact instancing
        /// link: https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-introduction
        void StoreMatrixRows3x4(const Matrix& m, DirectX::XMFLOAT4& r0, DirectX::XMFLOAT4& r1, DirectX::XMFLOAT4& r2)
        {
            using namespace DirectX;

            XMFLOAT4X4 tmp{};
            XMStoreFloat4x4(&tmp, m);

            // Row-major storage expected by your shaders.
            r0 = XMFLOAT4(tmp._11, tmp._12, tmp._13, tmp._14);
            r1 = XMFLOAT4(tmp._21, tmp._22, tmp._23, tmp._24);
            r2 = XMFLOAT4(tmp._31, tmp._32, tmp._33, tmp._34);

            // We intentionally omit row 4 (tmp._41.._44) for compact instancing.
        }
    } // namespace

    [[nodiscard]] TransformGpuData ToGpuData(const Transform& t)
    {
        using namespace DirectX;

        TransformGpuData out{};

        const Matrix world   = TransformMath::World(t);
        const Matrix normalM = TransformMath::WorldInverseTranspose(t);

        // row_major shaders => upload as-is (no transpose)
        XMStoreFloat4x4(&out.world, world);
        XMStoreFloat4x4(&out.normal, normalM);

        return out;
    }

    InstanceTransform3x4Gpu ToInstance3x4(const Transform& t)
    {
        InstanceTransform3x4Gpu out{};

        const Matrix world = TransformMath::World(t);
        StoreMatrixRows3x4(world, out.row0, out.row1, out.row2);

        return out;
    }
} // namespace Murder::TransformGpu
