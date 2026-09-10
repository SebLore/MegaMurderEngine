#pragma once

#include <vector>

#include <Core/MurderCore.hpp>

#include <Common/Scene/CameraComponent.h>

#include "Common/D3D11Headers.h"

namespace Murder
{

    enum class RenderViewType : uint8_t
    {
        Main,
        Shadow,
        Debug,
        Reflection,
        UIOverlay
    };
    struct RenderViewDesc
    {
        uint32_t       viewId = 0; // stays the same over frames
        RenderViewType type   = RenderViewType::Main;

        Murder::Camera camera;     // copied snapshot for this frame
        D3D11_VIEWPORT    viewport{}; // per-view viewport
        bool              clearColor = true;
        bool              clearDepth = true;

        // later: target handles, pass type, layer mask, jitter, etc.
    };

    struct RenderFramePacket
    {
        std::vector<RenderViewDesc> views;

        // std::vector<Renderable> draws;
        // std::vector<LightCpu> lights;

        // pass graph / pass commands
    };

} // namespace Murder
