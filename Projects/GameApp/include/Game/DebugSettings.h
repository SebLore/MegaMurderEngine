#pragma once

namespace Game
{
    // settings for debug mode
    struct DebugSettings
    {
        // -- additional drawing --
        bool useImgui     = false;
        bool startedImgui = false;
        bool useDebugDraw = false;
        bool drawZones    = false;

        // -- printing --
        bool printDebug = true;
    };
} // namespace Game
