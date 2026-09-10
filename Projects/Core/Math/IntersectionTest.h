#pragma once

#include "Types.h"

namespace Murder
{
    struct RayHitResult
    {
        bool    hit        = false;
        float   distance   = 0.0f;
        Vector3 entryPoint = { 0, 0, 0 };
    };

    inline Ray GetMouseRays(int mousePosX, int mousePosY, UINT windowWidth, UINT windowHeight)
    {
        // convert to ndc x-y coordinates
        float x = -1.0f + 2.0f * static_cast<float>(mousePosX) / static_cast<float>(windowWidth);
        float y = 1.0f - 2.0f * static_cast<float>(mousePosY) / static_cast<float>(windowHeight);

        // near is view-plane, far is max NDC in the z-axis
        return Ray{ { x, y, 0.0f }, { x, y, 1.0f } };
    }

    inline Ray RaysToWorldSpace(const Matrix& viewProj, const Ray& mouseRays)
    {
        Ray out = {};

        // inverse projection matrix to go from NDC to world space
        Matrix invViewProj = XMMatrixInverse(nullptr, viewProj);

        Vector3 farWorld = XMVector3TransformCoord(mouseRays.direction, invViewProj);

        out.position  = XMVector3TransformCoord(mouseRays.position, invViewProj);
        out.direction = DirectX::XMVector3Normalize(XMVectorSubtract(farWorld, out.position));

        return out;
    }

    inline RayHitResult TestRayAgainstBB(const DirectX::BoundingBox& bb, const Ray& ray)
    {
        RayHitResult result = {};

        if (bb.Intersects(ray.position, ray.direction, result.distance))
        {
            result.hit        = true;
            result.entryPoint = XMVectorAdd(ray.position, XMVectorScale(ray.direction, result.distance));
        }

        return result;
    }
} // namespace Murder