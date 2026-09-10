#pragma once

#include <DirectXColors.h>
#include <DirectXMath.h>

#include <algorithm> // for std::max and std::min

#include "DebugMath.h"

namespace dbg
{
    namespace details
    {
        inline constexpr size_t AxisLines    = 3;
        inline constexpr size_t AABBEdges    = 12;
        inline constexpr size_t FrustumEdges = 12;

        inline constexpr size_t SphereRings        = 3; // 3 orthogonal circles
        inline constexpr size_t ConeMultiplier     = 2; // base ring + spokes
        inline constexpr size_t CylinderMultiplier = 3; // 2 circles + cage lines
        inline constexpr size_t CapsuleMultiplier  = 4; // 4 arcs (2 per hemisphere)
        inline constexpr size_t GridLinesPerAxis   = 2; // 2 directions per grid

        inline constexpr size_t CapsuleExtraLongs = 4;
        inline constexpr size_t CapsuleExtraLats  = 8;

        /// Pairs of vertex indices that form the edges of a box, used for drawing AABBs. Each pair corresponds to an edge between two corners of the box.
        inline static constexpr uint8_t kBoxEdgePairs[12][2] = {
            { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, // near/bottom face loop
            { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, // far/top face loop
            { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }  // connecting edges
        };

        template <class EmitLine>
        void EmitBoxEdges(EmitLine&& emitLine, const DirectX::XMVECTOR corners[8], DirectX::FXMVECTOR color)
        {
            for (auto& edge : kBoxEdgePairs)
            {
                const uint8_t i0 = edge[0];
                const uint8_t i1 = edge[1];
                emitLine(corners[i0], corners[i1], color);
            }
        }

        inline void
        ComputeAABBCorners(DirectX::FXMVECTOR minCorner, DirectX::FXMVECTOR maxCorner, DirectX::XMVECTOR outCorners[8])
        {
            // Naming: minCorner/maxCorner rather than bmin/bmax etc.
            const float minX = DirectX::XMVectorGetX(minCorner);
            const float minY = DirectX::XMVectorGetY(minCorner);
            const float minZ = DirectX::XMVectorGetZ(minCorner);

            const float maxX = DirectX::XMVectorGetX(maxCorner);
            const float maxY = DirectX::XMVectorGetY(maxCorner);
            const float maxZ = DirectX::XMVectorGetZ(maxCorner);

            outCorners[0] = DirectX::XMVectorSet(minX, minY, minZ, 1);
            outCorners[1] = DirectX::XMVectorSet(maxX, minY, minZ, 1);
            outCorners[2] = DirectX::XMVectorSet(maxX, maxY, minZ, 1);
            outCorners[3] = DirectX::XMVectorSet(minX, maxY, minZ, 1);

            outCorners[4] = DirectX::XMVectorSet(minX, minY, maxZ, 1);
            outCorners[5] = DirectX::XMVectorSet(maxX, minY, maxZ, 1);
            outCorners[6] = DirectX::XMVectorSet(maxX, maxY, maxZ, 1);
            outCorners[7] = DirectX::XMVectorSet(minX, maxY, maxZ, 1);
        }

        template <class EmitLine, class PointAtT>
        void EmitPolyline(EmitLine&& emitLine, PointAtT&& pointAtT, int segments, DirectX::FXMVECTOR color)
        {
            // Generates segments by connecting successive points.
            DirectX::XMVECTOR prev = pointAtT(0);
            for (int i = 1; i <= segments; ++i)
            {
                DirectX::XMVECTOR cur = pointAtT(i);
                emitLine(prev, cur, color);
                prev = cur;
            }
        }
    } // namespace details

    /**
     * @brief Draw axes around a point, outputs the lines through the emitLine callback. X=red, Y=green, Z=blue.
     * @tparam EmitLine a callable type that takes (DirectX::FXMVECTOR a, DirectX::FXMVECTOR b, DirectX::FXMVECTOR color) and emits a line from a to b with the given color.
     * @param emitLine
     * @param origin
     * @param scale
     */
    template <class EmitLine>
    void Axis(EmitLine&& emitLine, DirectX::FXMVECTOR origin, float scale = 1.0f)
    {
        using namespace DirectX;
        emitLine(origin, XMVectorAdd(origin, XMVectorScale(g_XMIdentityR0, scale)), Colors::Red);
        emitLine(origin, XMVectorAdd(origin, XMVectorScale(g_XMIdentityR1, scale)), Colors::Blue);
        emitLine(origin, XMVectorAdd(origin, XMVectorScale(g_XMIdentityR2, scale)), Colors::Green);
    }

    /**
     * @brief Emits a ray in a given direction and length from an origin point
     * @tparam EmitLine
     * @param emitLine
     * @param origin
     * @param unitDirection
     * @param length
     * @param color
     */
    template <class EmitLine>
    void
    Ray(EmitLine&&         emitLine,
        DirectX::FXMVECTOR origin,
        DirectX::FXMVECTOR unitDirection,
        float              length,
        DirectX::FXMVECTOR color)
    {
        using namespace DirectX;
        emitLine(origin, XMVectorAdd(origin, XMVectorScale(unitDirection, length)), color);
    }

    template <class EmitLine>
    void Grid(
        EmitLine&&         emitLine,
        float              halfSize,
        int                divisions,
        DirectX::FXMVECTOR origin,
        DirectX::FXMVECTOR normal,
        DirectX::FXMVECTOR color)
    {
        using namespace DirectX;
        divisions = std::max(divisions, 1);

        // make sure normal is normalized
        XMVECTOR normalized = XMVector3Normalize(normal);

        // orthonormalize the plane
        XMVECTOR tangent, bitangent;
        BuildOrthonormalBasis(normalized, tangent, bitangent);

        float step = (2.0f * halfSize) / divisions;

        for (int i = 0; i <= divisions; ++i)
        {
            float off = -halfSize + i * step;

            // Line parallel to bitangent at offset along tangent
            {
                XMVECTOR a =
                    XMVectorAdd(origin, XMVectorAdd(XMVectorScale(tangent, off), XMVectorScale(bitangent, -halfSize)));
                XMVECTOR b =
                    XMVectorAdd(origin, XMVectorAdd(XMVectorScale(tangent, off), XMVectorScale(bitangent, halfSize)));
                emitLine(a, b, color);
            }

            // Line parallel to tangent at offset along bitangent
            {
                XMVECTOR a =
                    XMVectorAdd(origin, XMVectorAdd(XMVectorScale(tangent, -halfSize), XMVectorScale(bitangent, off)));
                XMVECTOR b =
                    XMVectorAdd(origin, XMVectorAdd(XMVectorScale(tangent, halfSize), XMVectorScale(bitangent, off)));
                emitLine(a, b, color);
            }
        }
    }

    template <class EmitLine>
    void GridXZ(EmitLine&& emitLine, float halfSize, int divisions, float y, DirectX::FXMVECTOR color)
    {
        using namespace DirectX;

        Grid(
            std::forward<EmitLine>(emitLine),
            halfSize,
            divisions,
            XMVectorSet(0, y, 0, 1),
            g_XMIdentityR1, // normal pointing up
            color);
    }

    template <class EmitLine>
    void AABB(EmitLine&& emitLine, DirectX::FXMVECTOR minCorner, DirectX::FXMVECTOR maxCorner, DirectX::FXMVECTOR color)
    {
        using namespace DirectX;

        XMVECTOR corners[8];

        details::ComputeAABBCorners(minCorner, maxCorner, corners);
        details::EmitBoxEdges(emitLine, corners, color);
    }

    template <class EmitLine>
    void AABBCenterExtents(
        EmitLine&&         emitLine,
        DirectX::FXMVECTOR center,
        DirectX::FXMVECTOR extents, // half-size in each axis
        DirectX::FXMVECTOR color)
    {
        using namespace DirectX;

        // Ensure extents are non-negative
        XMVECTOR halfSize = XMVectorAbs(extents);

        XMVECTOR minCorner = XMVectorSubtract(center, halfSize);
        XMVECTOR maxCorner = XMVectorAdd(center, halfSize);

        AABB(std::forward<EmitLine>(emitLine), minCorner, maxCorner, color);
    }

    template <class EmitLine>
    void Circle(
        EmitLine&&         emitLine,
        DirectX::FXMVECTOR center,
        DirectX::FXMVECTOR unitPlaneNormal,
        float              radius,
        int                segments,
        DirectX::FXMVECTOR color)
    {
        using namespace DirectX;
        segments = std::max(segments, 3); // minimum 3 segments for a closed shape

        // get tangent and bitangent
        XMVECTOR tangent, bitangent;
        BuildOrthonormalBasis(unitPlaneNormal, tangent, bitangent);

        auto pointAt = [&](int i)
        {
            float    angle  = (XM_2PI * i) / segments;
            XMVECTOR offset = XMVectorAdd(
                XMVectorScale(tangent, cosf(angle) * radius),
                XMVectorScale(bitangent, sinf(angle) * radius));
            return XMVectorAdd(center, offset);
        };

        details::EmitPolyline(std::forward<EmitLine>(emitLine), pointAt, segments, color);
    }

    template <class EmitLine>
    void
    Arc(EmitLine&&         emitLine,
        DirectX::FXMVECTOR center,
        DirectX::FXMVECTOR planeNormal,
        DirectX::FXMVECTOR startDirection,
        float              radius,
        float              angleRadians,
        int                segments,
        DirectX::FXMVECTOR color)
    {
        using namespace DirectX;

        segments = std::max(segments, 1);

        // normalized plane normal
        XMVECTOR n = XMVector3Normalize(planeNormal);

        // project start direction into plane and normalize
        XMVECTOR s = startDirection;

        // get component of s along n and subtract it to project s into the plane, then normalize
        s = XMVector3Normalize(XMVectorSubtract(s, XMVectorScale(n, XMVectorGetX(XMVector3Dot(s, n)))));

        // Since s is in the plane, rotation is just dir = s*cos(a) + (n×s)*sin(a)
        XMVECTOR nxs = XMVector3Cross(n, s);

        const float step = angleRadians / static_cast<float>(segments);

        // record last-but-one so we can draw a line from it
        XMVECTOR prev  = XMVectorAdd(center, XMVectorScale(s, radius));
        float    angle = 0.0f;

        for (int i = 1; i <= segments; i++)
        {
            angle += step;

            // get sin and cos from angle
            float sinA, cosA;
            XMScalarSinCos(&sinA, &cosA, angle);

            XMVECTOR dir = XMVectorAdd(XMVectorScale(s, cosA), XMVectorScale(nxs, sinA));
            XMVECTOR cur = XMVectorAdd(center, XMVectorScale(dir, radius));

            emitLine(prev, cur, color);

            // next line drawn from this point
            prev = cur;
        }
    }

    template <class EmitLine>
    void Sphere(EmitLine&& emitLine, DirectX::FXMVECTOR center, float radius, int segments, DirectX::FXMVECTOR color)
    {
        using namespace DirectX;
        Circle(emitLine, center, g_XMIdentityR0, radius, segments, color);
        Circle(emitLine, center, g_XMIdentityR1, radius, segments, color);
        Circle(emitLine, center, g_XMIdentityR2, radius, segments, color);
    }

    template <class EmitLine>
    void Cone(
        EmitLine&&         emitLine,
        DirectX::FXMVECTOR apex,
        DirectX::FXMVECTOR unitAxisDirection,
        float              angleRadians,
        float              length,
        int                segments,
        DirectX::FXMVECTOR color)
    {
        using namespace DirectX;
        segments = std::max(segments, 3);

        XMVECTOR baseCenter = XMVectorAdd(apex, XMVectorScale(unitAxisDirection, length));
        float    radius     = tanf(angleRadians) * length;

        Circle(emitLine, baseCenter, unitAxisDirection, radius, segments, color);

        XMVECTOR tangent, bitangent;
        BuildOrthonormalBasis(unitAxisDirection, tangent, bitangent);

        for (int i = 0; i < segments; ++i)
        {
            float ang = (XM_2PI * i) / segments;

            XMVECTOR p = XMVectorAdd(
                baseCenter,
                XMVectorAdd(XMVectorScale(tangent, cosf(ang) * radius), XMVectorScale(bitangent, sinf(ang) * radius)));

            emitLine(apex, p, color);
        }
    }

    template <class EmitLine>
    void Capsule(
        EmitLine&&         emitLine,
        DirectX::FXMVECTOR pointA,
        DirectX::FXMVECTOR pointB,
        float              radius,
        int                segments,
        DirectX::FXMVECTOR color)
    {
        using namespace DirectX;
        segments = std::max(segments, 6);

        XMVECTOR axis = XMVectorSubtract(pointB, pointA);
        float    len  = XMVectorGetX(XMVector3Length(axis));
        if (len < 1e-5f)
        {
            Sphere(emitLine, pointA, radius, segments, color);
            return;
        }

        XMVECTOR unitAxis = XMVector3Normalize(axis);
        XMVECTOR tangent, bitangent;
        BuildOrthonormalBasis(unitAxis, tangent, bitangent);

        // Rings at ends
        Circle(emitLine, pointA, unitAxis, radius, segments, color);
        Circle(emitLine, pointB, unitAxis, radius, segments, color);

        // Long lines (cylinder cage)
        int longs = 4;
        for (int i = 0; i < longs; ++i)
        {
            float    ang = (XM_2PI * i) / longs;
            XMVECTOR off =
                XMVectorAdd(XMVectorScale(tangent, cosf(ang) * radius), XMVectorScale(bitangent, sinf(ang) * radius));

            emitLine(XMVectorAdd(pointA, off), XMVectorAdd(pointB, off), color);
        }

        // Hemisphere-ish arcs
        Arc(emitLine, pointA, tangent, bitangent, radius, XM_PI, segments / 2, color);
        Arc(emitLine, pointA, bitangent, tangent, radius, XM_PI, segments / 2, color);
        Arc(emitLine, pointB, tangent, bitangent, radius, XM_PI, segments / 2, color);
        Arc(emitLine, pointB, bitangent, tangent, radius, XM_PI, segments / 2, color);
    }

    template <class EmitLine>
    void FrustumFromInvViewProj(EmitLine&& emitLine, DirectX::CXMMATRIX invViewProj, DirectX::FXMVECTOR color)
    {
        using namespace DirectX;

        const XMVECTOR ndc[8] = {
            XMVectorSet(-1, -1, 0, 1), XMVectorSet(1, -1, 0, 1), XMVectorSet(1, 1, 0, 1), XMVectorSet(-1, 1, 0, 1),
            XMVectorSet(-1, -1, 1, 1), XMVectorSet(1, -1, 1, 1), XMVectorSet(1, 1, 1, 1), XMVectorSet(-1, 1, 1, 1),
        };

        XMVECTOR worldCorners[8];
        for (int i = 0; i < 8; ++i)
        {
            const XMVECTOR p = XMVector4Transform(ndc[i], invViewProj);
            worldCorners[i]  = XMVectorScale(p, 1.0f / XMVectorGetW(p));
        }

        details::EmitBoxEdges(std::forward<EmitLine>(emitLine), worldCorners, color);
    }

    template <class EmitLine>
    void Cylinder(
        EmitLine&&         emitLine,
        DirectX::FXMVECTOR base,
        DirectX::FXMVECTOR normal,
        float              height,
        float              radius,
        int                segments,
        DirectX::FXMVECTOR color)
    {
        // draw two circles x number of segments oriented by the normal, offset by the height
        Circle(emitLine, base, normal, radius, segments, color);
        Circle(
            emitLine,
            DirectX::XMVectorAdd(base, DirectX::XMVectorScale(normal, height)),
            normal,
            radius,
            segments,
            color);

        // get tangent and bitangent
        DirectX::XMVECTOR tangent, bitangent;
        BuildOrthonormalBasis(normal, tangent, bitangent);

        // combine them at each segment to form the "cage" of the cylinder
        for (int i = 0; i < segments; ++i)
        {
            float             ang = (DirectX::XM_2PI * i) / segments;
            DirectX::XMVECTOR off = DirectX::XMVectorAdd(
                DirectX::XMVectorScale(tangent, cosf(ang) * radius),
                DirectX::XMVectorScale(bitangent, sinf(ang) * radius));
            emitLine(
                DirectX::XMVectorAdd(base, off),
                DirectX::XMVectorAdd(base, DirectX::XMVectorAdd(off, DirectX::XMVectorScale(normal, height))),
                color);
        }
    }

} // namespace dbg
