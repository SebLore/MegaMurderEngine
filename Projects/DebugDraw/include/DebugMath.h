#pragma once

#include <DirectXMath.h>

/**
 * @brief Given a normalized unit normal vector, builds an orthonormal basis (tangent and bitangent) around it.
 * @param normal normalized unit normal vector to build the basis around
 * @param tangent output tangent vector, normalized and perpendicular to normal
 * @param bitangent output bitangent vector, normalized and perpendicular to both normal and tangent
 */
inline void BuildOrthonormalBasis(DirectX::FXMVECTOR normal, DirectX::XMVECTOR& tangent, DirectX::XMVECTOR& bitangent)
{
    using namespace DirectX;

    // Pick helper axis not parallel to normal
    XMVECTOR axis = (fabsf(XMVectorGetY(normal)) < 0.999f) ? g_XMIdentityR1 : g_XMIdentityR0;

    tangent   = XMVector3Normalize(XMVector3Cross(axis, normal));
    bitangent = XMVector3Normalize(XMVector3Cross(normal, tangent));
}
