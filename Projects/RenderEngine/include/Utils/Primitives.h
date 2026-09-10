/*****************************************************************
 * @file   Primitives.h
 * @brief Platform agnostic factory functions for generatic 3D primitives, using type-traits to determine vertex composition
 *
 * Currently supports:
 * - cube
 * - skybox(inverted cube
 * - uvsphere
 * - grid
 *
 * Credit to nickdesaulniers for the original code: https://github.com/nickdesaulniers/prims/
 * @author SebLore
 * @date   August 2025
 *********************************************************************/
#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include <type_traits>
namespace
{
    // helper function for is_same_v with multiple types
    template <typename T, typename... Ts>
    constexpr bool is_any_same = (std::is_same_v<T, Ts> || ...);

// helper macro to make the if-statemment cleaner
#undef SAME
#define SAME(T, ...) constexpr(is_any_same<T, __VA_ARGS__>)
} // namespace

namespace Primitives3D
{
    struct P
    {
        static constexpr uint8_t floatCount = 3;
    }; // position
    struct PN
    {
        static constexpr uint8_t floatCount = 6;
    }; // position + normal
    struct PU
    {
        static constexpr uint8_t floatCount = 6;
    }; // position + uv
    struct PUN
    {
        static constexpr uint8_t floatCount = 8;
    }; // position + uv + normal
    struct PRGB
    {
        static constexpr uint8_t floatCount = 6;
    }; // position + rgb color
    struct PRGBA
    {
        static constexpr uint8_t floatCount = 7;
    }; // position + rgba color

    template <typename T>
    struct vertex_traits
    {
        static constexpr size_t vertexCount = 0;  // default to 0
        static constexpr size_t indexCount  = 36; // always 36 indices for a cube
    };

    template <>
    struct vertex_traits<P>
    {
        static constexpr size_t vertexCount = 24;
    };

    template <>
    struct vertex_traits<PN>
    {
        static constexpr size_t vertexCount = 24;
    };

    template <>
    struct vertex_traits<PU>
    {
        static constexpr size_t vertexCount = 24;
    };

    template <>
    struct vertex_traits<PUN>
    {
        static constexpr size_t vertexCount = 24;
    };

    template <>
    struct vertex_traits<PRGB>
    {
        static constexpr size_t vertexCount = 8;
    };

    template <>
    struct vertex_traits<PRGBA>
    {
        static constexpr size_t vertexCount = 8;
    };

    // cube indices for a clockwise winding order
    static constexpr std::array<unsigned int, 36> cubeIndices = {
        0,  1,  2,  0,  2,  3,  // front
        4,  5,  6,  4,  6,  7,  // back
        8,  9,  10, 8,  10, 11, // left
        12, 13, 14, 12, 14, 15, // right
        16, 17, 18, 16, 18, 19, // bottom
        20, 21, 22, 20, 22, 23, // top
    };

    // base cube vertices, centered at origin, side length 2, used to generate other vertex types
    static constexpr std::array<std::array<float, 3>, 8> base = { {
        { -1.0f, -1.0f, -1.0f },
        { -1.0, 1.0f, -1.0f },
        { 1.0f, 1.0f, -1.0f },
        { 1.0f, -1.0f, -1.0f }, // 0-3 front face BL TL TR BR facing Z-
        { -1.0f, -1.0f, 1.0f },
        { -1.0, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, -1.0f, 1.0f } // 4-7 back face BL TL TR BR facing Z+
    } };

    // indices of the 4 corners that make up each face
    static constexpr std::array<std::array<int, 4>, 6> faceCorners = { {
        { 0, 1, 2, 3 }, // Z- BL TL TR BR
        { 7, 6, 5, 4 }, // Z+
        { 4, 5, 1, 0 }, // X-
        { 3, 2, 6, 7 }, // X+
        { 4, 0, 3, 7 }, // Y-
        { 1, 5, 6, 2 }, // Y+
    } };

    /// @brief Creates vertices for a cube with the given side length
    /// @tparam VertexType Type of the vertex, defaults to PUN (position, uv, normal)
    /// @param half Half the side length of the cube
    /// @param uvsPerFace if true, each face of the cube has its own 0-1 uvs
    /// @param clockwise setting this to true makes the cube's faces face outwards. If false , the faces face inwards, for skyboxes.
    /// @return
    template <typename VertexType = PUN>
    inline constexpr std::array<float, vertex_traits<VertexType>::vertexCount * VertexType::floatCount>
    GenerateCubeVertices(float half, bool uvsPerFace = false, bool clockwise = true)
    {
        constexpr size_t vcount = vertex_traits<VertexType>::vertexCount;
        constexpr size_t fcount = VertexType::floatCount;

        // array to hold the generated vertices
        std::array<float, vcount * fcount> vertices{};

        // generate vertices one face at a time
        size_t idx = 0;
        for (size_t face = 0; face < faceCorners.size(); ++face)
        {
            for (size_t corner = 0; corner < faceCorners[face].size(); ++corner)
            {
                // position
                {
                    // get the position from the base cube and scale it by the half side length
                    const auto& pos = base[faceCorners[face][corner]];
                    vertices[idx++] = pos[0] * half; // x
                    vertices[idx++] = pos[1] * half; // y
                    vertices[idx++] = pos[2] * half; // z
                } // position

                // add other attributes depending on vertex type

                // UVs
                if SAME (VertexType, PU, PUN)
                {
                    float u = 0.0f;
                    float v = 0.0f;

                    if (uvsPerFace)
                    {
                        // per-face, [1:0]x[0:1], 0:0 TL 1:1 BR
                        // different UVs for each face (so each face can have its own texture space)
                        if (corner == 1)
                            v = 1.0f;
                        else if (corner == 2) // TL or TR
                        {
                            u = 1.0f;
                            v = 1.0f;
                        }
                        else if (corner == 3) // BR or BL
                            u = 0.0f;
                    }
                    else
                    {
                        // global UVs for the whole cube, mapped as a cross-shape
                        switch (face)
                        {
                        case 0: // front Z-
                            u = (corner == 0) ? 0.333f : (corner == 1) ? 0.333f : (corner == 2) ? 0.666f : 0.666f;
                            v = (corner == 0) ? 0.666f : (corner == 1) ? 1.0f : (corner == 2) ? 1.0f : 0.666f;
                            break;
                        case 1: // back Z+
                            u = (corner == 0) ? 0.666f : (corner == 1) ? 0.666f : (corner == 2) ? 0.333f : 0.333f;
                            v = (corner == 0) ? 0.666f : (corner == 1) ? 1.0f : (corner == 2) ? 1.0f : 0.666f;
                            break;
                        case 2: // left X-
                            u = (corner == 0) ? 0.0f : (corner == 1) ? 0.0f : (corner == 2) ? 0.333f : 0.333f;
                            v = (corner == 0) ? 0.666f : (corner == 1) ? 1.0f : (corner == 2) ? 1.0f : 0.666f;
                            break;
                        case 3: // right X+
                            u = (corner == 0) ? 0.666f : (corner == 1) ? 0.666f : (corner == 2) ? 1.0f : 1.0f;
                            v = (corner == 0) ? 0.666f : (corner == 1) ? 1.0f : (corner == 2) ? 1.0f : 0.666f;
                            break;
                        case 4: // bottom Y-
                            u = (corner == 0) ? 0.333f : (corner == 1) ? 0.333f : (corner == 2) ? 0.666f : 0.666f;
                            v = (corner == 0) ? 0.333f : (corner == 1) ? 0.666f : (corner == 2) ? 0.666f : 0.333f;
                            break;
                        case 5: // top Y+
                            u = (corner == 0) ? 0.333f : (corner == 1) ? 0.333f : (corner == 2) ? 0.666f : 0.666f;
                            v = (corner == 0) ? 1.0f : (corner == 1) ? 0.666f : (corner == 2) ? 0.666f : 1.0f;
                            break;
                        }
                    }

                    // flip UVs for clockwise vs counter-clockwise
                    if (!clockwise)
                        u = 1.0f - u;

                    vertices[idx++] = u;
                    vertices[idx++] = v;
                }

                // normals
                if SAME (VertexType, PN, PUN)
                {
                    float nx = 0.0f;
                    float ny = 0.0f;
                    float nz = 0.0f;

                    nx = (face == 2) ? -1.0f : (face == 3) ? 1.0f : 0.0f; // X-component
                    ny = (face == 4) ? -1.0f : (face == 5) ? 1.0f : 0.0f; // Y-component
                    nz = (face == 0) ? -1.0f : (face == 1) ? 1.0f : 0.0f; // Z-component

                    // invert normals if not clockwise
                    if (!clockwise)
                    {
                        nx = -nx;
                        ny = -ny;
                        nz = -nz;
                    }

                    vertices[idx++] = nx;
                    vertices[idx++] = ny;
                    vertices[idx++] = nz;
                }
            }
        }
        return vertices;
    }

    constexpr auto GenerateCubeIndices(bool clockwise)
    {
        if (clockwise)
            return cubeIndices;
        else
        {
            std::array<unsigned int, 36> reversed{};
            for (size_t i = 0; i < cubeIndices.size() / 3; i++)
            {
                reversed[i * 3 + 0] = cubeIndices[i * 3 + 0];
                reversed[i * 3 + 1] = cubeIndices[i * 3 + 2];
                reversed[i * 3 + 2] = cubeIndices[i * 3 + 1];
            }
            return reversed;
        }
    }

    template <typename VertexType = PUN>
    inline void GenerateCube(
        std::vector<float>&    outVertices,
        std::vector<unsigned>& outIndices,
        float                  sideLength,
        bool                   uvsPerFace = false,
        bool                   clockwise  = true)
    {
        auto vertices = GenerateCubeVertices<VertexType>(sideLength, uvsPerFace, clockwise);
        auto indices  = GenerateCubeIndices(clockwise);

        outVertices.assign(vertices.begin(), vertices.end());
        outIndices.assign(indices.begin(), indices.end());
    }

    template <typename VertexType = PUN>
    inline void GenerateSkyboxCube(
        std::vector<float>&    outVertices,
        std::vector<unsigned>& outIndices,
        float                  sideLength,
        bool                   uvsPerFace = false)
    {
        GenerateCube<VertexType>(outVertices, outIndices, sideLength, uvsPerFace, false);
    }

#include "DirectXMath.h" // TODO: use agnostic data type instead but cba to do the operations myself right now

    inline DirectX::XMFLOAT3 MakeUVSperePos(float radius, float phi, float theta)
    {
        const float s = sinf(phi);

        // spherical to cartesian coordinate conversion
        return DirectX::XMFLOAT3(radius * s * cosf(theta), radius * cosf(phi), radius * s * sinf(theta));
    }

    inline DirectX::XMFLOAT3 MakeUVSphereSmoothNormal(const DirectX::XMFLOAT3& p)
    {
        DirectX::XMVECTOR v = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&p));
        DirectX::XMFLOAT3 n;
        DirectX::XMStoreFloat3(&n, v);
        return n;
    }

    inline DirectX::XMFLOAT3
    MakeFlatNormal(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& p2)
    {
        DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&p0);
        DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&p1);
        DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&p2);
        DirectX::XMVECTOR u  = DirectX::XMVectorSubtract(v1, v0);
        DirectX::XMVECTOR v  = DirectX::XMVectorSubtract(v2, v0);
        DirectX::XMVECTOR n  = DirectX::XMVector3Cross(u, v);
        n                    = DirectX::XMVector3Normalize(n);
        DirectX::XMFLOAT3 normal;
        DirectX::XMStoreFloat3(&normal, n);

        return normal;
    }

    inline void
    MakeUVSphereNormals(const DirectX::XMFLOAT3 pos[], DirectX::XMFLOAT3 norm[], bool smoothed, unsigned count = 4)
    {
        DirectX::XMFLOAT3 normal = MakeFlatNormal(pos[0], pos[1], pos[2]);

        for (unsigned int i = 0; i < count; ++i)
            norm[i] = smoothed ? MakeUVSphereSmoothNormal(pos[i]) : normal;
    }

    inline DirectX::XMFLOAT2
    MakeSphereUVs(float phiStep, float thetaStep, unsigned sliceCount, unsigned i, unsigned j, bool rightEdge)
    {
        const float phi   = static_cast<float>(i) * phiStep;
        float       theta = static_cast<float>(j) * thetaStep;

        // Seam fix:
        if (rightEdge && j == sliceCount)
            return DirectX::XMFLOAT2(1.0f, phi / DirectX::XM_PI);

        float u = theta / DirectX::XM_2PI; // [0,1)
        float v = phi / DirectX::XM_PI;    // [0,1]
        return DirectX::XMFLOAT2(u, v);
    }

    // TODO: allow for any vertex type and vector of floats, now just PUN
    /// @brief Generate a UV sphere mesh by generating vertices evenly distributed around a central point
    /// by some factor of pi using spherical coordinates. Slides control the number of vertical divisions while stacks
    /// control the number of horizontal divisions.
    /// @tparam VertexType Type of vertex to generate, defaults to PUN (position, uv, normal)
    /// @param outVertices Output vector to hold the generated vertices
    /// @param outIndices Output vector to hold the indices
    /// @param radius radius of the sphere in world/model space
    /// @param sliceCount number of slices (longitudinal divisions). More slices = smoother surface.
    /// @param stackCount number of stacks (latitudinal divisions). The more the smoother the surface of the sphere is
    template <typename VertexType = PUN>
    inline void GenerateUVSphere(
        std::vector<float>&    outVertices,
        std::vector<unsigned>& outIndices,
        float                  radius,
        unsigned int           sliceCount,
        unsigned int           stackCount,
        bool                   smoothed    = true,
        bool                   rightHanded = true)
    {
        outVertices.clear();
        outIndices.clear();
        std::vector<float>        vertices;
        std::vector<unsigned int> indices;
        //std::unordered_map<size_t, size_t> vertexMap; // map to avoid duplicate vertices, key: hash of vertex, value: index in vertices vector TODO: use this, make hash function for each vertex type

        size_t newSize = static_cast<size_t>(stackCount) * sliceCount * 6 * vertex_traits<VertexType>::vertexCount *
                         VertexType::floatCount;
        vertices.reserve(newSize); // 6 vertices per quad

        // step sizes along the sphere
        const float phiStep   = DirectX::XM_PI / static_cast<float>(stackCount);
        const float thetaStep = DirectX::XM_2PI / static_cast<float>(sliceCount);

        for (unsigned int i = 0; i < stackCount; ++i)
        {
            // phi0 and phi1 determine the angles for the current stack
            const float phi0 = static_cast<float>(i) * phiStep;
            const float phi1 = static_cast<float>(i + 1) * phiStep;

            // using DirectXMath for easier vector math :)
            DirectX::XMFLOAT3 pos[4]   = {};
            DirectX::XMFLOAT3 norms[4] = {};
            DirectX::XMFLOAT2 uvs[4]   = {};

            for (unsigned int j = 0; j < sliceCount; ++j)
            {
                // theta0 and theta1 determine the angles for the current slice
                // together with phi0 and phi1 they define a quad on the sphere surface
                const float theta0 = static_cast<float>(j) * thetaStep;
                const float theta1 = static_cast<float>(j + 1) * thetaStep;

                pos[0] = MakeUVSperePos(radius, phi0, theta0);
                pos[1] = MakeUVSperePos(radius, phi1, theta0);
                pos[2] = MakeUVSperePos(radius, phi1, theta1);
                pos[3] = MakeUVSperePos(radius, phi0, theta1);

                MakeUVSphereNormals(pos, norms, smoothed);

                uvs[0] = MakeSphereUVs(phiStep, thetaStep, sliceCount, i, j, false);
                uvs[1] = MakeSphereUVs(phiStep, thetaStep, sliceCount, i + 1, j, false);
                uvs[2] = MakeSphereUVs(phiStep, thetaStep, sliceCount, i + 1, j + 1, true);
                // right edge of the sphere
                uvs[3] = MakeSphereUVs(phiStep, thetaStep, sliceCount, i, j + 1, true);
                // right edge of the sphere

                if (rightHanded)
                {
                    // swap pos1 and pos3 to change winding order
                    std::swap(pos[1], pos[3]);
                    std::swap(norms[1], norms[3]);
                    std::swap(uvs[1], uvs[3]);
                }

                // add two triangles per quad (6 vertices)
                for (unsigned int k = 0; k < 6; ++k)
                {
                    unsigned int index = (k == 0)   ? 0
                                         : (k == 1) ? 1
                                         : (k == 2) ? 2
                                         : (k == 3) ? 0
                                         : (k == 4) ? 2
                                         : (k == 5) ? 3
                                                    : 0;
                    // position
                    vertices.push_back(pos[index].x);
                    vertices.push_back(pos[index].y);
                    vertices.push_back(pos[index].z);
                    // uv
                    vertices.push_back(
                        (index == 0)   ? uvs[0].x
                        : (index == 1) ? uvs[1].x
                        : (index == 2) ? uvs[2].x
                        : (index == 3) ? uvs[3].x
                                       : 0.0f);
                    vertices.push_back(
                        (index == 0)   ? uvs[0].y
                        : (index == 1) ? uvs[1].y
                        : (index == 2) ? uvs[2].y
                        : (index == 3) ? uvs[3].y
                                       : 0.0f);
                    // normal
                    vertices.push_back(norms[index].x);
                    vertices.push_back(norms[index].y);
                    vertices.push_back(norms[index].z);
                    indices.push_back(static_cast<unsigned int>(indices.size()));
                }
            }
        }
        vertices.shrink_to_fit();
        //outVertices.assign(vertices.begin(), vertices.end());
        outVertices = std::move(vertices);
        outIndices  = std::move(indices);
    }

    /* Plane */

    /// @brief Create a plane with given width and depth
    /// @tparam VertexType Type of vertex to generate, defaults to PUN (position, uv, normal)
    /// @param outVertices vector to hold generated vertices, floats
    /// @param outIndices vector to hold generated indices, unsigned ints
    /// @param width of the plane
    /// @param depth of the plane
    /// @param uvs whether to generate uvs
    /// @param rightHanded whether the plane is right handed (clockwise winding order)
    /// @param flip_UVs if true bottom left is coordinate space origin, else top left is origin
    template <typename VertexType = PUN>
    inline void GeneratePlane(
        std::vector<float>&    outVertices,
        std::vector<unsigned>& outIndices,
        float                  width,
        float                  depth,
        bool                   uvs         = true,
        bool                   rightHanded = true,
        bool                   flip_UVs    = false)
    {
        std::vector<float>    vertices;
        std::vector<unsigned> indices;

        const float halfWidth = width * 0.5f;
        const float halfDepth = depth * 0.5f;

        // positions
        const DirectX::XMFLOAT3 pos[4] = {
            { -halfWidth, 0.0f, -halfDepth }, // 0 BL
            { -halfWidth, 0.0f, halfDepth },  // 1 TL
            { halfWidth, 0.0f, halfDepth },   // 2 TR
            { halfWidth, 0.0f, -halfDepth }   // 3 BR
        };

        // normals
        DirectX::XMFLOAT3 normal = { 0.0f, 1.0f, 0.0f };
        if (!rightHanded)
            normal = { 0.0f, -1.0f, 0.0f };
        const DirectX::XMFLOAT3 norms[4] = { normal, normal, normal, normal };
        // uvs
        DirectX::XMFLOAT2 uvsArr[4]      = {
            { 0.0f, 0.0f }, // 0 TL
            { 0.0f, 1.0f }, // 1 BL
            { 1.0f, 0.0f }, // 2 TR
            { 1.0f, 1.0f }  // 3 BR
        };

        // swap uvs for clockwise vs counter-clockwise
        if (!rightHanded)
            std::swap(uvsArr[1], uvsArr[3]);

        if (flip_UVs)
        {
            // flip V coordinate
            uvsArr[0].y = 1.0f - uvsArr[0].y;
            uvsArr[1].y = 1.0f - uvsArr[1].y;
            uvsArr[2].y = 1.0f - uvsArr[2].y;
            uvsArr[3].y = 1.0f - uvsArr[3].y;
        }

        // build vertices
        for (size_t i = 0; i < 4; ++i)
        {
            // position
            vertices.push_back(pos[i].x);
            vertices.push_back(pos[i].y);
            vertices.push_back(pos[i].z);
            // uv
            if SAME (VertexType, PU, PUN)
            {
                vertices.push_back(uvsArr[i].x);
                vertices.push_back(uvsArr[i].y);
            }
            // normal
            if SAME (VertexType, PN, PUN)
            {
                vertices.push_back(norms[i].x);
                vertices.push_back(norms[i].y);
                vertices.push_back(norms[i].z);
            }
        }

        // build indices
        if (rightHanded)
        {
            // clockwise
            indices = { 0, 1, 2, 0, 2, 3 };
        }
        else
        {
            // counter-clockwise
            indices = { 0, 2, 1, 0, 3, 2 };
        }

        outVertices = std::move(vertices);
        outIndices  = std::move(indices);
    }

    template <typename VertexType = PUN>
    inline void GenerateGridPlane(
        std::vector<float>&    outVertices,
        std::vector<unsigned>& outIndices,
        unsigned int           gridCountX,
        unsigned int           gridCountZ,
        float                  width,
        float                  depth,
        bool                   uvs         = true,
        bool                   rightHanded = true,
        bool                   flip_UVs    = false)
    {
        std::vector<float>    vertices;
        std::vector<unsigned> indices;

        const float halfWidth = width * 0.5f;
        const float halfDepth = depth * 0.5f;

        const float dx = width / static_cast<float>(gridCountX); // step in x direction
        const float dz = depth / static_cast<float>(gridCountZ); // step in z direction
        const float du = 1.0f / static_cast<float>(gridCountX);  // uv step in u direction
        const float dv = 1.0f / static_cast<float>(gridCountZ);  // uv step in v direction

        // build vertices
        for (unsigned int i = 0; i <= gridCountZ; ++i)
        {
            float z = halfDepth - i * dz; // current z position
            for (unsigned int j = 0; j <= gridCountX; ++j)
            {
                float x = -halfWidth + j * dx; // current x position
                // position
                vertices.push_back(x);
                vertices.push_back(0.0f);
                vertices.push_back(z);

                // uv
                if SAME (VertexType, PU, PUN)
                {
                    float u = j * du;
                    float v = i * dv;
                    if (flip_UVs)
                        v = 1.0f - v;
                    vertices.push_back(u);
                    vertices.push_back(v);
                }

                // normal
                if SAME (VertexType, PN, PUN)
                {
                    DirectX::XMFLOAT3 normal = { 0.0f, 1.0f, 0.0f };
                    if (!rightHanded)
                        normal = { 0.0f, -1.0f, 0.0f };
                    vertices.push_back(normal.x);
                    vertices.push_back(normal.y);
                    vertices.push_back(normal.z);
                }
            }
        }

        // build indices
        for (unsigned int i = 0; i < gridCountZ; ++i)
        {
            for (unsigned int j = 0; j < gridCountX; ++j)
            {
                unsigned int row1 = i * (gridCountX + 1);
                unsigned int row2 = (i + 1) * (gridCountX + 1);
                if (rightHanded)
                {
                    // clockwise
                    indices.push_back(row1 + j);     // 0
                    indices.push_back(row1 + j + 1); // 1
                    indices.push_back(row2 + j);     // 2

                    indices.push_back(row2 + j);     // 2
                    indices.push_back(row1 + j + 1); // 1
                    indices.push_back(row2 + j + 1); // 3
                }
                else
                {
                    // counter-clockwise
                    indices.push_back(row1 + j);     // 0
                    indices.push_back(row2 + j);     // 2
                    indices.push_back(row1 + j + 1); // 1

                    indices.push_back(row2 + j);     // 2
                    indices.push_back(row2 + j + 1); // 3
                    indices.push_back(row1 + j + 1); // 1
                }
            }
        }
        outVertices = std::move(vertices);
        outIndices  = std::move(indices);
    }
} // namespace Primitives3D

#undef SAME // undefine helper macro
