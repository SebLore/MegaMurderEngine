#include "DebugLineRenderer.h"

#include <Utility/ErrorHandling.h>

void DebugLineRenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    assert(device && context && "Primitive Batch needs both device and context");

    m_Context = context;

    m_States = std::make_unique<DirectX::CommonStates>(device);
    m_Batch  = std::make_unique<DirectX::PrimitiveBatch<Vtx>>(context);

    m_Effect = std::make_unique<DirectX::BasicEffect>(device);
    m_Effect->SetVertexColorEnabled(true);

    void const* shaderByteCode = nullptr;
    size_t      byteCodeLength = 0;
    m_Effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    HRESULT hr = device->CreateInputLayout(
        Vtx::InputElements,
        Vtx::InputElementCount,
        shaderByteCode,
        byteCodeLength,
        m_InputLayout.ReleaseAndGetAddressOf());

    if (FAILED(hr))
        throw std::runtime_error("CreateInputLayout failed");
}

void DebugLineRenderer::Begin(DirectX::CXMMATRIX view, DirectX::CXMMATRIX proj) const
{
    m_Effect->SetWorld(DirectX::XMMatrixIdentity());
    m_Effect->SetView(view);
    m_Effect->SetProjection(proj);

    m_Context->IAGetPrimitiveTopology(&m_PrevTopology);
    m_HasPrevTopology = true;

    m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    m_Context->OMSetBlendState(m_States->Opaque(), nullptr, 0xFFFFFFFF);
    m_Context->RSSetState(m_States->CullNone());

    if (m_Depth)
        m_Context->OMSetDepthStencilState(m_States->DepthRead(), 0);
    else
        m_Context->OMSetDepthStencilState(m_States->DepthNone(), 0);

    m_Effect->Apply(m_Context.Get());
    m_Context->IASetInputLayout(m_InputLayout.Get());

    m_Batch->Begin();
}

void DebugLineRenderer::End() const
{
    m_Batch->End();
    if (m_HasPrevTopology)
    {
        m_Context->IASetPrimitiveTopology(m_PrevTopology);
        m_HasPrevTopology = false;
    }
}

void DebugLineRenderer::Line(DirectX::FXMVECTOR a, DirectX::FXMVECTOR b, DirectX::FXMVECTOR color) const
{
    m_Batch->DrawLine(Vtx(a, color), Vtx(b, color));
}

void DebugLineRenderer::SetEnableDepth(bool enable) { m_Depth = enable; }

bool DebugLineRenderer::Depth() const { return m_Depth; }
