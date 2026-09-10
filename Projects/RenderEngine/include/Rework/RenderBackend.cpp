// RenderBackend.cpp

#include "RenderBackend.h"

#include <Common/Assets/MeshAsset.h>

#include "Rework/VertexLayout.h"

#include <ResourceManagement/Loaders/TextureLoader.h>

#include <Utility/ErrorHandling.h>
#include <Utility/Logging.h>

#define LOG_TAG "RenderBackend"

namespace Murder::Render
{
    namespace
    {
        // prepend function signature
#define RENDER_BACKEND_MSG(msg) (std::string(__func__) + ": " + std::string(msg))
    } // namespace

    RenderBackend::RenderBackend(ID3D11Device* device, ID3D11DeviceContext* context)
        : m_Device(device), m_Context(context), m_GpuLoader(device)
    {
        THROWIA_IF(!m_Device, RENDER_BACKEND_MSG("device was nullptr"));
        THROWIA_IF(!m_Context, RENDER_BACKEND_MSG("context was nullptr"));

        uint8_t rgba[4]  = { 255, 255, 255, 255 };
        m_DefaultTexture = std::make_shared<DX::TextureSRV>(TextureLoader::CreateColorTexture(m_Device, rgba, 1, 1));
    }

    const MeshGpu* RenderBackend::FindMesh(MeshId id) const noexcept
    {
        auto it = m_Meshes.find(id);
        if (it == m_Meshes.end())
            return nullptr;

        return &it->second;
    }

    bool RenderBackend::HasMesh(MeshId id) const noexcept { return FindMesh(id); }

    bool RenderBackend::HasTexture(TextureId id) const noexcept { return FindTexture(id); }

    const MeshGpu& RenderBackend::GetOrCreateMesh(MeshId id, const MeshAsset& mesh)
    {
        if (auto it = m_Meshes.find(id); it != m_Meshes.end())
            return it->second;

        MeshGpu      gpu{};
        CreateResult result = CreateMeshGpu(mesh, gpu);
        if (!result.success)
            result.error ? THROWRE(result.error->c_str()) : THROWRE("unknown error creating mesh gpu");

        auto [it, _] = m_Meshes.emplace(id, std::move(gpu));
        return it->second;
    }

    const std::shared_ptr<DX::TextureSRV>* RenderBackend::FindTexture(TextureId id) const noexcept
    {
        auto it = m_Textures.find(id);
        if (it == m_Textures.end())
            return nullptr;

        return &it->second;
    }

    const std::shared_ptr<DX::TextureSRV>&
    RenderBackend::GetOrCreateTexture(TextureId id, const TextureUploadDesc& upload)
    {
        if (auto it = m_Textures.find(id); it != m_Textures.end())
            return it->second;

        if (m_TextureLoadFailed.contains(id))
            return m_DefaultTexture;

        auto texture = m_GpuLoader.CreateTexture(upload);
        if (!texture)
        {
            m_TextureLoadFailed.insert(id);
            LOG_WARN(
                "Texture upload failed id=" << Core::ToHexString(id.value) << " key='" << upload.debugName
                                            << "' source='" << upload.sourcePath
                                            << "' bytes=" << upload.bytes.size_bytes());
            return m_DefaultTexture;
        }
        texture->SetDebugName(std::string(upload.debugName));

        LOG_DEBUG(
            "Texture upload success id=" << Core::ToHexString(id.value) << " key='" << upload.debugName << "' source='"
                                         << upload.sourcePath << "' bytes=" << upload.bytes.size_bytes());

        auto [it, _] = m_Textures.emplace(id, std::move(texture));
        return it->second;
    }

    RenderBackend::CreateResult RenderBackend::CreateMeshGpu(const MeshAsset& mesh, MeshGpu& out) const
    {
        CreateResult result{};
        out = {};

        if (mesh.layout.stride == 0)
        {
            result.error = RENDER_BACKEND_MSG("mesh.layout.stride == 0");
            return result;
        }

        if (mesh.vertexData.empty())
        {
            result.error = RENDER_BACKEND_MSG("mesh.vertexData is empty");
            return result;
        }

        if ((mesh.vertexData.size() % mesh.layout.stride) != 0)
        {
            result.error = RENDER_BACKEND_MSG("vertexData size is not divisible by stride");
            return result;
        }

        out.layout       = mesh.layout;
        out.vertexStride = mesh.layout.stride;
        out.vertexCount  = static_cast<uint32_t>(mesh.vertexData.size() / mesh.layout.stride);

        out.indexCount  = static_cast<uint32_t>(mesh.indices.size());
        out.indexFormat = DXGI_FORMAT_R32_UINT;
        out.topology    = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        out.submeshes.reserve(mesh.submeshes.size());
        for (const auto& sm : mesh.submeshes)
        {
            SubMeshGpu g{};
            g.indexStart = sm.indexStart;
            g.indexCount = sm.indexCount;
            g.baseVertex = sm.baseVertex;
            out.submeshes.push_back(g);
        }

        // Vertex Buffer
        {
            D3D11_BUFFER_DESC desc{};
            desc.Usage     = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.ByteWidth = static_cast<UINT>(mesh.vertexData.size());

            D3D11_SUBRESOURCE_DATA init{};
            init.pSysMem = mesh.vertexData.data();

            const HRESULT hr = m_Device->CreateBuffer(&desc, &init, out.vertexBuffer.GetAddressOf());
            if (FAILED(hr) || !out.vertexBuffer)
            {
                result.error = RENDER_BACKEND_MSG("CreateBuffer(VB) failed");
                return result;
            }
        }

        // Index Buffer
        // In the case where Blender outputs vertices without indices
        if (!mesh.indices.empty())
        {
            D3D11_BUFFER_DESC desc{};
            desc.Usage     = D3D11_USAGE_DEFAULT; // TODO: support immutable
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            desc.ByteWidth = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t)); // TODO: support 16-bit indices

            D3D11_SUBRESOURCE_DATA init{};
            init.pSysMem = mesh.indices.data();

            const HRESULT hr = m_Device->CreateBuffer(&desc, &init, out.indexBuffer.GetAddressOf());
            if (FAILED(hr) || !out.indexBuffer)
            {
                result.error = RENDER_BACKEND_MSG("CreateBuffer(IB) failed");
                return result;
            }
        }

        result.success = true;
        return result;
    }

    void RenderBackend::BindMesh(const MeshGpu& mesh) const
    {
        THROWIA_IF(!mesh.vertexBuffer, RENDER_BACKEND_MSG("BindMesh: vertexBuffer is null"));
        THROWIA_IF(mesh.vertexStride == 0, RENDER_BACKEND_MSG("BindMesh: vertexStride is 0"));

        ID3D11Buffer* vb     = mesh.vertexBuffer.Get();
        const UINT    stride = mesh.vertexStride;
        const UINT    offset = 0;

        m_Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

        if (mesh.indexBuffer && mesh.indexCount > 0)
            m_Context->IASetIndexBuffer(mesh.indexBuffer.Get(), mesh.indexFormat, 0);
        else
            m_Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

        m_Context->IASetPrimitiveTopology(mesh.topology);
    }

    void RenderBackend::DrawMesh(const MeshGpu& mesh) const
    {
        if (mesh.HasIndices())
        {
            if (!mesh.submeshes.empty())
                for (const auto& sm : mesh.submeshes)
                    m_Context->DrawIndexed(sm.indexCount, sm.indexStart, sm.baseVertex);
            else
                m_Context->DrawIndexed(mesh.indexCount, 0, 0);
        }
        else
        {
            // Debug fallback: draw non-indexed
            m_Context->Draw(mesh.vertexCount, 0);
        }
    }

    void RenderBackend::DrawSubMesh(const MeshGpu& mesh, uint32_t submeshIndex) const
    {
        if (!mesh.HasIndices())
        {
            m_Context->Draw(mesh.vertexCount, 0);
            return;
        }

        if (submeshIndex < mesh.submeshes.size())
        {
            const auto& sm = mesh.submeshes[submeshIndex];
            m_Context->DrawIndexed(sm.indexCount, sm.indexStart, sm.baseVertex);
            return;
        }

        m_Context->DrawIndexed(mesh.indexCount, 0, 0);
    }

    ID3D11InputLayout* RenderBackend::GetOrCreateInputLayout(MeshId meshId, const MeshGpu& mesh, const DX::Shader& vs)
    {
        const void* byteCode = vs.GetByteCode();
        const auto  bcSize   = vs.GetByteCodeSize();
        if (!byteCode || bcSize == 0)
            return nullptr;

        const InputLayoutKey key{ .meshId = meshId.value, .vsByteCode = byteCode, .vsByteCodeSize = bcSize };

        if (const auto it = m_InputLayouts.find(key); it != m_InputLayouts.end())
            return it->second.Get();

        if (m_InputLayoutCreateFailed.contains(key))
            return nullptr;

        try
        {
            std::vector<D3D11_INPUT_ELEMENT_DESC> descs = Murder::DxInput::Build(mesh.layout);

            ComPtr<ID3D11InputLayout> created;
            const HRESULT             hr = m_Device->CreateInputLayout(
                descs.data(),
                static_cast<UINT>(descs.size()),
                byteCode,
                bcSize,
                created.GetAddressOf());

            if (SUCCEEDED(hr) && created)
            {
                auto [it, _] = m_InputLayouts.emplace(key, std::move(created));
                return it->second.Get();
            }

            LOG_ERROR("Input layout creation failed for meshId=" << Core::ToHexString(meshId.value));
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(
                "Input layout exception for meshId=" << Core::ToHexString(meshId.value) << " reason=" << e.what());
        }

        m_InputLayoutCreateFailed.insert(key);
        return nullptr;
    }
} // namespace Murder::Render

// undef
#undef RENDER_BACKEND_MSG
