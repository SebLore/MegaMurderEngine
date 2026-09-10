/**
* @file Mesh.h
*
* A class to wrap a mesh's vertex and index buffers, as well as its submesh information and its
* bounding box data.
*/
#pragma once

#include <memory>

#include <DirectXCollision.h> // for bounding box
#include <d3d11.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include "Abstraction/IndexBuffer.h"
#include "Abstraction/VertexBuffer.h"

// Submesh structure
struct SubMesh
{
    UINT        indexStart = 0;
    UINT        indexCount = 0;
    std::string mtl;
};

// Mesh interface
class IMesh
{
  public:
    IMesh()                                                                                         = default;
    virtual ~IMesh()                                                                                = default;
    virtual void                        Bind(ID3D11DeviceContext* context) const                    = 0;
    virtual void                        UnBind(ID3D11DeviceContext* context) const                  = 0;
    virtual void                        Draw(ID3D11DeviceContext* context) const                    = 0;
    virtual void                        DrawSubmesh(ID3D11DeviceContext* context, UINT index) const = 0;
    virtual const DirectX::BoundingBox& GetBoundingBox() const                                      = 0;
    virtual void                        SetBoundingBox(const DirectX::BoundingBox& box)             = 0;
    virtual UINT                        SubmeshCount() const                                        = 0;
    virtual const std::string&          GetMaterialID(size_t submeshIndex) const                    = 0;
    virtual const std::string&          GetMaterialLibID() const                                    = 0;
};

// Mesh class, vertex type is templated
template <typename VertexType>
class Mesh : public IMesh
{
  public:
    Mesh()           = default;
    ~Mesh() override = default;
    Mesh(Mesh&& right);
    Mesh& operator=(Mesh&& right);

    Mesh(
        ID3D11Device*               device,
        const VertexType*           vertices,
        UINT                        vertexCount,
        const UINT*                 indices,
        UINT                        indexCount,
        const std::vector<SubMesh>& submeshes = std::vector<SubMesh>(),
        DirectX::XMFLOAT3           min_p     = { FLT_MAX, FLT_MAX, FLT_MAX },
        DirectX::XMFLOAT3           max_p     = { -FLT_MAX, -FLT_MAX, -FLT_MAX },
        const std::string&          mtllib    = "");
    Mesh(
        ID3D11Device*               device,
        const float*                vertices,
        UINT                        vertexSize,
        UINT                        vertexCount,
        const unsigned int*         indices,
        UINT                        indexCount,
        const std::vector<SubMesh>& submeshes = std::vector<SubMesh>(),
        DirectX::XMFLOAT3           min_p     = { FLT_MAX, FLT_MAX, FLT_MAX },
        DirectX::XMFLOAT3           max_p     = { -FLT_MAX, -FLT_MAX, -FLT_MAX },
        const std::string&          mtllib    = "");

    void Initialize(
        ID3D11Device*               device,
        const VertexType*           vertexData,
        UINT                        vertexCount,
        const UINT*                 indexData,
        UINT                        indexCount,
        const std::vector<SubMesh>& submeshes = std::vector<SubMesh>(),
        DirectX::XMFLOAT3           min_p     = { FLT_MAX, FLT_MAX, FLT_MAX },
        DirectX::XMFLOAT3           max_p     = { -FLT_MAX, -FLT_MAX, -FLT_MAX },
        const std::string&          mtllib    = "");

    void Initialize(
        ID3D11Device*               device,
        const float*                vertices,
        unsigned int                vertexSize,
        unsigned int                vertexCount,
        const unsigned int*         indices,
        unsigned int                indexCount,
        const std::vector<SubMesh>& submeshes = std::vector<SubMesh>(),
        DirectX::XMFLOAT3           min_p     = { FLT_MAX, FLT_MAX, FLT_MAX },
        DirectX::XMFLOAT3           max_p     = { -FLT_MAX, -FLT_MAX, -FLT_MAX },
        const std::string&          mtllib    = "");

    // void Update(ID3D11DeviceContext* context, float dt = 0.0f);
    void Bind(ID3D11DeviceContext* context) const override;
    void UnBind(ID3D11DeviceContext* context) const override;
    void DrawSubmesh(ID3D11DeviceContext* context, UINT index) const override;
    void Draw(ID3D11DeviceContext* context) const override;

    const DirectX::BoundingBox& GetBoundingBox() const override { return m_boundingBox; }
    void                        SetBoundingBox(const DirectX::BoundingBox& box) override { m_boundingBox = box; }

    UINT SubmeshCount() const override { return static_cast<UINT>(m_submeshes.size()); }

    const std::vector<SubMesh>& GetSubmeshes() const { return m_submeshes; }
    const std::string&          GetMaterialID(size_t submeshIndex) const override
    {
        THROWOOR_IF(submeshIndex > m_submeshes.size(), "out of range");
        return m_submeshes.at(submeshIndex).mtl;
    }
    const std::string& GetMaterialLibID() const override { return m_mtllib; }

    // no copying
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

  private:
    DX::VertexBuffer<VertexType> m_vertexBuffer; // vertex buffer
    DX::IndexBuffer              m_indexBuffer;  // index buffer

    bool                 m_hasSubmeshes = false;
    std::vector<SubMesh> m_submeshes;

    DirectX::BoundingBox              m_boundingBox;   // base bounding box for the entire mesh
    std::vector<DirectX::BoundingBox> m_boundingBoxes; // bounding boxes for all the submeshes
    std::string                       m_mtllib;
};

template <typename VertexType>
Mesh<VertexType>::Mesh(Mesh&& right)
{
    m_vertexBuffer = std::move(right.m_vertexBuffer);
    m_indexBuffer  = std::move(right.m_indexBuffer);

    m_hasSubmeshes = right.m_hasSubmeshes;
    m_submeshes    = std::move(right.m_submeshes);

    m_boundingBox   = right.m_boundingBox;
    m_boundingBoxes = std::move(right.m_boundingBoxes);

    m_mtllib = std::move(right.m_mtllib);

    // reset right
    right.m_vertexBuffer.Reset();
    right.m_indexBuffer.Reset();

    right.m_hasSubmeshes = false;
    right.m_submeshes.clear();
    right.m_boundingBoxes.clear();
}

template <typename VertexType>
Mesh<VertexType>& Mesh<VertexType>::operator=(Mesh&& right)
{
    if (this != &right)
    {
        m_vertexBuffer = std::move(right.m_vertexBuffer);
        m_indexBuffer  = std::move(right.m_indexBuffer);

        m_hasSubmeshes = right.m_hasSubmeshes;
        m_submeshes    = std::move(right.m_submeshes);

        m_boundingBox   = right.m_boundingBox;
        m_boundingBoxes = std::move(right.m_boundingBoxes);

        m_mtllib = std::move(right.m_mtllib);

        // reset right
        right.m_vertexBuffer.Reset();
        right.m_indexBuffer.Reset();

        right.m_hasSubmeshes = false;
        right.m_submeshes.clear();
        right.m_boundingBoxes.clear();
    }
    return *this;
}

template <typename VertexType>
Mesh<VertexType>::Mesh(
    ID3D11Device*               device,
    const VertexType*           vertices,
    UINT                        vertexCount,
    const UINT*                 indices,
    UINT                        indexCount,
    const std::vector<SubMesh>& submeshes,
    DirectX::XMFLOAT3           min_p,
    DirectX::XMFLOAT3           max_p,
    const std::string&          mtllib)
{
    Initialize(device, vertices, vertexCount, indices, indexCount, submeshes, min_p, max_p, mtllib);
}

template <typename VertexType>
Mesh<VertexType>::Mesh(
    ID3D11Device*               device,
    const float*                vertices,
    UINT                        vertexSize,
    UINT                        vertexCount,
    const unsigned int*         indices,
    UINT                        indexCount,
    const std::vector<SubMesh>& submeshes,
    DirectX::XMFLOAT3           min_p,
    DirectX::XMFLOAT3           max_p,
    const std::string&          mtllib)
{
    Initialize(device, vertices, vertexSize, vertexCount, indices, indexCount, submeshes, min_p, max_p, mtllib);
}

/**
 * @brief Initialize the mesh with vertex and index data
 * @tparam T vertex type
 * @param device valid ID3D11Device ptr
 * @param vertexData pointer to vertex data
 * @param vertexCount number of vertices
 * @param indexData pointer to index data
 * @param indexCount number of indices
 * @param submeshes vector of submeshes to initialize the mesh with
 */
template <typename VertexType>
void Mesh<VertexType>::Initialize(
    ID3D11Device*               device,
    const VertexType*           vertexData,
    UINT                        vertexCount,
    const UINT*                 indexData,
    UINT                        indexCount,
    const std::vector<SubMesh>& submeshes,
    DirectX::XMFLOAT3           min_p,
    DirectX::XMFLOAT3           max_p,
    const std::string&          mtllib)
{
    m_vertexBuffer.Initialize(device, vertexData, vertexCount);
    m_indexBuffer.Initialize(device, indexData, sizeof(UINT), indexCount);
    m_submeshes.assign(submeshes.begin(), submeshes.end());
    if (!m_submeshes.empty())
        m_hasSubmeshes = true;

    // create canonical bounding box
    DirectX::BoundingBox::CreateFromPoints(m_boundingBox, DirectX::XMLoadFloat3(&min_p), DirectX::XMLoadFloat3(&max_p));

    m_mtllib = mtllib;
}

/// @brief Initializes a mesh from a const array of floats
/// @tparam T The vertex type to be used
/// @param device valid decide
/// @param vertices pointer to the vertex data
/// @param vertexSize byte size of one vertex
/// @param vertexCount number of vertices (byte size/number of floats in vertices)
/// @param indices pointer to index data
/// @param indexCount number of indices
/// @param submeshes list of submeshes
/// @param min_p min point for the mesh, used for bounding box
/// @param max_p max point for the mesh,       =||=
template <typename VertexType>
void Mesh<VertexType>::Initialize(
    ID3D11Device*               device,
    const float*                vertices,
    unsigned int                vertexSize,
    unsigned int                vertexCount,
    const unsigned int*         indices,
    unsigned int                indexCount,
    const std::vector<SubMesh>& submeshes,
    DirectX::XMFLOAT3           min_p,
    DirectX::XMFLOAT3           max_p,
    const std::string&          mtllib)
{
    // calculate new vertex count based on  size
    UINT vtxCount = vertexCount * sizeof(float) / vertexSize;

    m_vertexBuffer.Initialize(device, vertices, vtxCount);
    m_indexBuffer.Initialize(device, indices, sizeof(UINT), indexCount);
    m_submeshes.assign(submeshes.begin(), submeshes.end());
    if (!m_submeshes.empty())
        m_hasSubmeshes = true;

    DirectX::BoundingBox::CreateFromPoints(m_boundingBox, DirectX::XMLoadFloat3(&min_p), DirectX::XMLoadFloat3(&max_p));

    m_mtllib = mtllib;
}

/**
 * @brief Bind the mesh to the input assembler stage
 * @param context valid ID3D11DeviceContext ptr
 */
template <typename VertexType>
void Mesh<VertexType>::Bind(ID3D11DeviceContext* context) const
{
    THROWIA_IF(!context, "context was nullptr");

    m_vertexBuffer.Bind(context);
    m_indexBuffer.Bind(context);
}

/**
*	@brief Unbind the mesh from the input assembler stage
*	@param context valid ID3D11DeviceContext ptr
*/
template <typename VertexType>
void Mesh<VertexType>::UnBind(ID3D11DeviceContext* context) const
{
    THROWIA_IF(!context, "context was nullptr");
    m_vertexBuffer.UnBind(context);
    m_indexBuffer.UnBind(context);
}

/**
 * @brief Draw a specific submesh
 * @param context ID3D11DeviceContext ptr
 * @param index index of the submesh
 */
template <typename VertexType>
void Mesh<VertexType>::DrawSubmesh(ID3D11DeviceContext* context, UINT index) const
{
    THROWOOR_IF(index >= m_submeshes.size(),
                "Submesh index out of range"); // sanity check
    THROWIA_IF(!context, "context was nullptr");

    context->DrawIndexed(m_submeshes[index].indexCount, m_submeshes[index].indexStart, 0);
}

template <typename VertexType>
void Mesh<VertexType>::Draw(ID3D11DeviceContext* context) const
{
    THROWIA_IF(!context, "context was nullptr");

    if (!m_submeshes.empty())
        for (const auto& submesh : m_submeshes)
            context->DrawIndexed(submesh.indexCount, submesh.indexStart, 0);
    else
        context->DrawIndexed(m_indexBuffer.GetIndexCount(), 0, 0);
}
