// AssimpMeshExtract.h
#pragma once

#include <string>

#include <assimp/mesh.h>

#include <Common/Assets/MeshAsset.h> // Murder::MeshCpu

namespace Murder
{
    struct VertexBuildOptions
    {
        bool wantNormals  = true;
        bool wantUV0      = true;
        bool wantTangents = false; // requires aiProcess_CalcTangentSpace
        bool wantColors0  = false;

        // Skinning can be added later (needs more formats in VertexFormat).
        bool wantSkinning = false;
    };

    bool BuildMeshCpuFromAssimp(const aiMesh* mesh, MeshAsset& out, const VertexBuildOptions& opt, std::string& err);
} // namespace Murder
