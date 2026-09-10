#pragma once

#include <Common/Scene/CameraComponent.h>

namespace Murder
{

    struct alignas(16) CameraGpuData
    {
        DirectX::XMFLOAT4X4 view{};
        DirectX::XMFLOAT4X4 proj{};
        DirectX::XMFLOAT4X4 viewProj{};

        DirectX::XMFLOAT3 eyePosition{ 0.0f, 0.0f, 0.0f };
        float             pad;
    };

    // assert to create compile error instead of encountering undefined errors
    static_assert((sizeof(CameraGpuData) % 16) == 0, "CameraGpuData must be 16-byte sized for D3D11 cbuffer");

    namespace CameraMath
    {
        /// Packs CPU camera into GPU cbuffer data, creating matrices
        inline CameraGpuData ToGpuData(const Camera& cam)
        {
            using namespace DirectX;

            CameraGpuData out{};

            const Matrix view     = cam.View();
            const Matrix proj     = cam.Projection();
            const Matrix viewProj = view * proj; // avoid extra calculation

            // row_major shaders => upload as-is (no transpose)
            XMStoreFloat4x4(&out.view, view);
            XMStoreFloat4x4(&out.proj, proj);
            XMStoreFloat4x4(&out.viewProj, viewProj);

            out.eyePosition = cam.position;
            out.pad         = 0.0f;

            return out;
        }
    } // namespace CameraMath

} // namespace Murder
