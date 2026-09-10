#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")  // ensure this is defined so naming debug objects works