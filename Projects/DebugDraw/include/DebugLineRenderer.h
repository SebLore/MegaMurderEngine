#pragma once

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <memory>

// DirectXTK
#include <CommonStates.h>
#include <Effects.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>

class DebugLineRenderer
{
  public:
    using Vtx = DirectX::VertexPositionColor;

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);

    void Begin(DirectX::CXMMATRIX view, DirectX::CXMMATRIX proj) const;
    void End() const;

    void Line(DirectX::FXMVECTOR a, DirectX::FXMVECTOR b, DirectX::FXMVECTOR color) const;

    void SetEnableDepth(bool enable = true);
    bool Depth() const;

  private:
    ComPtr<ID3D11DeviceContext> m_Context = nullptr;

    std::unique_ptr<DirectX::CommonStates>        m_States;
    std::unique_ptr<DirectX::BasicEffect>         m_Effect;
    std::unique_ptr<DirectX::PrimitiveBatch<Vtx>> m_Batch;

    
    ComPtr<ID3D11InputLayout> m_InputLayout; ///< should match the vertex we want to draw (VertexPositionColor)

    // storage
    bool m_Depth = false;

    // topology state backup
    mutable D3D11_PRIMITIVE_TOPOLOGY m_PrevTopology    = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    mutable bool                     m_HasPrevTopology = false;
};
