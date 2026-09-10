// AssimpMeshExtract.cpp
#include "AssimpMeshExtract.h"

#include <Core/Math/AABB.h> // Murder::AABB

#include <algorithm>
#include <vector>
#include <array>
#include <cstring>
#include <memory>

namespace Murder
{

    namespace
    {
        uint16_t FormatSize(VertexFormat f)
        {
            switch (f)
            {
            case VertexFormat::FLOAT1:
                return 4;
            case VertexFormat::FLOAT2:
                return 8;
            case VertexFormat::FLOAT3:
                return 12;
            case VertexFormat::FLOAT4:
                return 16;

            case VertexFormat::UINT1:
                return 4;
            case VertexFormat::UINT2:
                return 8;
            case VertexFormat::UINT3:
                return 12;
            case VertexFormat::UINT4:
                return 16;

            case VertexFormat::UBYTE4:
                return 4;
            case VertexFormat::USHORT4:
                return 8;
            default:
                return 0;
            }
        }

        template <class T>
        void Write(void* dst, const T& v)
        {
            std::memcpy(dst, &v, sizeof(T));
        }

        void AddElem(VertexLayout& layout, VertexSemantic sem, uint8_t semIdx, VertexFormat fmt, uint8_t inputSlot)
        {
            VertexElement e{};
            e.semantic      = sem;
            e.semanticIndex = semIdx;
            e.format        = fmt;
            e.inputSlot     = inputSlot;
            e.offset        = layout.stride;

            const uint16_t sz = FormatSize(fmt);
            layout.elements.push_back(e);
            layout.stride = static_cast<uint16_t>(layout.stride + sz);
        }

        // Returns a new index buffer with flipped winding order.
        std::vector<uint32_t> FlipTriangleIndexWinding(const std::vector<uint32_t>& indices)
        {
            if (indices.size() % 3 != 0)
                throw std::invalid_argument("Index buffer length must be a multiple of 3 (triangulated).");

            std::vector<uint32_t> out = indices;
            for (size_t i = 0; i < out.size(); i += 3)
                std::swap(out[i + 1], out[i + 2]);

            return out;
        }
    } // namespace

    bool BuildMeshCpuFromAssimp(const aiMesh* mesh, MeshAsset& out, const VertexBuildOptions& opt, std::string& err)
    {
        err.clear();

        if (!mesh)
        {
            err = "aiMesh was null";
            return false;
        }
        if (!mesh->HasPositions() || mesh->mNumVertices == 0)
        {
            err = "aiMesh has no positions/vertices";
            return false;
        }

        // ---- build layout based on what the mesh actually has ----
        VertexLayout layout{};
        layout.elements.clear();
        layout.stride = 0;

        AddElem(layout, VertexSemantic::POSITION, 0, VertexFormat::FLOAT3, 0);

        const bool hasNormals  = opt.wantNormals && mesh->HasNormals();
        const bool hasUV0      = opt.wantUV0 && mesh->HasTextureCoords(0);
        const bool hasTangents = opt.wantTangents && mesh->HasTangentsAndBitangents();
        const bool hasColors0  = opt.wantColors0 && mesh->HasVertexColors(0);

        if (opt.wantNormals)
            AddElem(layout, VertexSemantic::NORMAL, 0, VertexFormat::FLOAT3, 0);

        if (opt.wantUV0)
            AddElem(layout, VertexSemantic::TEXCOORD, 0, VertexFormat::FLOAT2, 0);

        if (hasTangents)
        {
            AddElem(layout, VertexSemantic::TANGENT, 0, VertexFormat::FLOAT3, 0);
            AddElem(layout, VertexSemantic::BITANGENT, 0, VertexFormat::FLOAT3, 0);
        }

        if (hasColors0)
            AddElem(layout, VertexSemantic::COLOR, 0, VertexFormat::FLOAT4, 0);

        std::vector<std::array<uint32_t, 4>> joints;
        std::vector<std::array<float, 4>>    weights;

        if (opt.wantSkinning && mesh->HasBones())
        {
            AddElem(layout, VertexSemantic::JOINTS, 0, VertexFormat::UINT4, 0);
            AddElem(layout, VertexSemantic::WEIGHTS, 0, VertexFormat::FLOAT4, 0);

            const uint32_t vCount = mesh->mNumVertices;
            joints.resize(vCount, { 0, 0, 0, 0 });
            weights.resize(vCount, { 0, 0, 0, 0 });

            for (unsigned b = 0; b < mesh->mNumBones; ++b)
            {
                const aiBone* bone = mesh->mBones[b];
                if (!bone)
                    continue;

                for (unsigned w = 0; w < bone->mNumWeights; ++w)
                {
                    const aiVertexWeight& vw = bone->mWeights[w];
                    if (vw.mVertexId >= vCount)
                        continue;

                    auto& j  = joints[vw.mVertexId];
                    auto& wt = weights[vw.mVertexId];

                    int minIdx = 0;
                    for (int k = 1; k < 4; ++k)
                        if (wt[k] < wt[minIdx])
                            minIdx = k;

                    if (vw.mWeight > wt[minIdx])
                    {
                        wt[minIdx] = vw.mWeight;
                        j[minIdx]  = b;
                    }
                }
            }

            for (uint32_t i = 0; i < vCount; ++i)
            {
                float s = weights[i][0] + weights[i][1] + weights[i][2] + weights[i][3];
                if (s > 0.0f)
                {
                    weights[i][0] /= s;
                    weights[i][1] /= s;
                    weights[i][2] /= s;
                    weights[i][3] /= s;
                }
                else
                {
                    weights[i] = { 1, 0, 0, 0 };
                }
            }
        }

        // ---- allocate output ----
        out.layout = layout;
        out.vertexData.clear();
        out.indices.clear();
        out.submeshes.clear();
        out.bounds.reset();

        const uint32_t vCount = mesh->mNumVertices;
        out.vertexData.resize(static_cast<size_t>(vCount) * out.layout.stride);
        out.positions.resize(static_cast<size_t>(vCount));

        //if (opt.wantPositions)
        //{
        //    out.geometryData = std::make_unique<MeshGeometryData>();
        //    out.geometryData->positions.resize(vCount);
        //}

        // ---- fill vertex data + bounds ----
        AABB aabb;
        aabb.Reset();

        // iterate per vertex and fill in the raw data
        for (uint32_t i = 0; i < vCount; ++i)
        {
            std::byte* base = out.vertexData.data() + static_cast<size_t>(i) * out.layout.stride;

            for (const VertexElement& e : out.layout.elements)
            {
                void* dst = base + e.offset;

                switch (e.semantic)
                {
                case VertexSemantic::POSITION:
                {
                    const aiVector3D& p = mesh->mVertices[i];

                    Vector3 pos{ p.x, p.y, p.z };

                    out.positions[i] = pos;

                    Write(dst, pos);
                    aabb.Expand(pos);
                }
                break;

                case VertexSemantic::NORMAL:
                {
                    Vector3 norm{ 0, 1, 0 };
                    if (hasNormals)
                    {
                        const aiVector3D& aiNorm = mesh->mNormals[i];

                        norm = Vector3{ aiNorm.x, aiNorm.y, aiNorm.z };
                    }
                    Write(dst, norm);
                }
                break;

                case VertexSemantic::TEXCOORD:
                {
                    Vector2 uv{ 0, 0 };
                    if (hasUV0)
                    {
                        const aiVector3D& t = mesh->mTextureCoords[0][i];
                        uv                  = Vector2{ t.x, t.y };
                    }
                    Write(dst, uv);
                }
                break;

                case VertexSemantic::TANGENT:
                {
                    Vector3 t{ 1, 0, 0 };
                    if (hasTangents)
                    {
                        const aiVector3D& tt = mesh->mTangents[i];

                        t = Vector3{ tt.x, tt.y, tt.z };
                    }
                    Write(dst, t);
                }
                break;

                case VertexSemantic::BITANGENT:
                {
                    Vector3 b{ 0, 0, 1 };
                    if (hasTangents)
                    {
                        const aiVector3D& bb = mesh->mBitangents[i];
                        b                    = Vector3{ bb.x, bb.y, bb.z };
                    }
                    Write(dst, b);
                }
                break;

                case VertexSemantic::COLOR:
                {
                    Vector4 c{ 1, 1, 1, 1 };
                    if (hasColors0)
                    {
                        const aiColor4D& cc = mesh->mColors[0][i];
                        c                   = Vector4{ cc.r, cc.g, cc.b, cc.a };
                    }
                    Write(dst, c);
                }
                break;

                case VertexSemantic::JOINTS:
                {
                    const auto& j = joints[i];
                    Write(dst, j);
                }
                break;

                case VertexSemantic::WEIGHTS:
                {
                    const auto& w = weights[i];
                    Write(dst, w);
                }
                break;

                default:
                    break;
                }
            }
        }

        // ---- indices ----
        out.indices.reserve(static_cast<size_t>(mesh->mNumFaces) * 3);

        for (unsigned f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3)
                continue;

            out.indices.push_back(face.mIndices[0]);
            out.indices.push_back(face.mIndices[1]);
            out.indices.push_back(face.mIndices[2]);
        }

        out.invIndices = FlipTriangleIndexWinding(out.indices);

        // ---- one submesh covering the full aiMesh ----
        SubMeshCpu sm{};
        sm.indexStart = 0;
        sm.indexCount = static_cast<uint32_t>(out.indices.size());
        sm.baseVertex = 0;
        sm.name       = mesh->mName.C_Str();

        if (aabb.IsValid())
        {
            out.bounds = aabb;
            sm.bounds  = aabb;
        }

        out.submeshes.push_back(std::move(sm));

        return true;
    }
} // namespace Murder
