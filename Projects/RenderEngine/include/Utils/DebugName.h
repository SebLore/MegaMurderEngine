#pragma once

#include <Common/D3D11Headers.h>

#ifdef _DEBUG
#include <d3d11sdklayers.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#endif

#include <wrl/client.h>
#include <string>

namespace DX::Debug
{
#ifdef _DEBUG

    inline void SetDebugName(ID3D11DeviceChild* obj, const char* name)
    {
        if (obj && name)
            obj->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(std::strlen(name)), name);
    }

    inline void SetDebugName(IDXGIObject* obj, const char* name)
    {
        if (obj && name)
            obj->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(std::strlen(name)), name);
    }

    template <typename TComPtr>
    void SetDebugName(const Microsoft::WRL::ComPtr<TComPtr>& obj, const char* name)
    {
        SetDebugName(obj.Get(), name);
    }

    inline std::string IndexedName(const char* base, size_t index)
    {
        return std::string(base) + "[" + std::to_string(index) + "]";
    }

    inline void ReportLiveD3DObjects(ID3D11Device* device, bool ignoreInternal = true)
    {
        if (!device)
            return;

        Microsoft::WRL::ComPtr<ID3D11Debug> debug;
        if (FAILED(device->QueryInterface(IID_PPV_ARGS(&debug))) || !debug)
            return;

        UINT flags = D3D11_RLDO_DETAIL;
        if (ignoreInternal)
            flags |= D3D11_RLDO_IGNORE_INTERNAL;

        debug->ReportLiveDeviceObjects(static_cast<D3D11_RLDO_FLAGS>(flags));
    }

    inline void ReportLiveDXGIObjects()
    {
        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        if (FAILED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))) || !debug)
            return;

        debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
    }

#else

    inline void SetDebugName(ID3D11DeviceChild*, const char*) {}
    inline void SetDebugName(IDXGIObject*, const char*) {}

    template <typename T>
    inline void SetDebugName(const T&, const char*)
    {
    }

    inline void ReportLiveD3DObjects(ID3D11Device*, bool = true) {}
    inline void ReportLiveDXGIObjects() {}

#endif
} // namespace DX::Debug
