/***************************************************************************/ /**
 * @file   Vertices.h
 * @brief  Defines a few common vertex structures with input layouts for D3D11.
 * 
 * @author Sebastian L
 * @date   December 2023
 ******************************************************************************/

#pragma once

#include <d3d11.h>

#include <DirectXMath.h>

struct VertexP
{
    DirectX::XMFLOAT3 position;

    static constexpr UINT                     inputElementCount              = 1;
    static constexpr D3D11_INPUT_ELEMENT_DESC inputLayout[inputElementCount] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
};

struct VertexPN
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;

    constexpr VertexPN(
        DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f },
        DirectX::XMFLOAT3 normal   = { 0.0f, 0.0f, 1.0f })
        : position(position), normal(normal)
    {
    }
    constexpr VertexPN(float x, float y, float z, float nx, float ny, float nz) : position(x, y, z), normal(nx, ny, nz)
    {
    }

    static constexpr UINT                     inputElementCount              = 2;
    static constexpr D3D11_INPUT_ELEMENT_DESC inputLayout[inputElementCount] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
};

/*
* Vertex_PosUVNorm
*
* Vertex structure for a vertex with position, uv and normal
* Has an input layout for easier pairing with vertex shaders
*/
struct VertexPUN
{
    DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT2 uv       = { 0.0f, 0.0f };
    DirectX::XMFLOAT3 normal   = { 0.0f, 0.0f, 1.0f };

    constexpr VertexPUN(
        DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f },
        DirectX::XMFLOAT2 uv       = { 0.0f, 0.0f },
        DirectX::XMFLOAT3 normal   = { 0.0f, 0.0f, 1.0f })
        : position(position), uv(uv), normal(normal)
    {
    }

    constexpr VertexPUN(float x, float y, float z, float u, float v, float nx, float ny, float nz)
        : position(x, y, z), uv(u, v), normal(nx, ny, nz)
    {
    }
    static constexpr UINT ByteSize() { return static_cast<UINT>(sizeof(position) + sizeof(uv) + sizeof(normal)); };

    static constexpr UINT                     inputElementCount              = 3;
    static constexpr D3D11_INPUT_ELEMENT_DESC inputLayout[inputElementCount] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
}; // default vertex type

/*
* Vertex_PUV
*
* Vertex structure for a vertex with a position and uv coordinates
*/
struct VertexPU
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 uv;

    static constexpr UINT inputElementCount = 2;

    static constexpr D3D11_INPUT_ELEMENT_DESC inputLayout[inputElementCount] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
};
