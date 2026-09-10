#pragma once

#include <ECS.hpp>

#include "Game/Components.h"

namespace Game
{
    using namespace Murder;
    using ECS::Entity;

    /// trigger whenever a shot happens
    struct ShootEvent
    {
        Entity     shooter{};
        Vector3    origin{};
        Vector3    dir{};
        WeaponType type{};
    };

    /// Trigger when a shot hits something
    struct HitEvent
    {
        Entity target{};
        float  damage{};
    };

    enum class Sfx : uint8_t
    {
        PISTOL_SHOT,
        SHOTGUN_SHOT,
        ENEMY_IDLE,
        ENEMY_HIT,
        ENEMY_DIE,
        STEP
    };

    struct AudioEvent
    {
        std::string key;
    };

    //struct SFX
    //{
    //    Sfx sfx;
    //};

    struct ZoneEvent
    {
        Entity zone;
        Entity entity;
    };

    // recording all events per-frame
    // todo: only for now, replace with event system, use templates for event types
    struct FrameEvents
    {
        std::vector<ShootEvent> shots;
        std::vector<HitEvent>   hits;
        std::vector<AudioEvent> audio;
        std::vector<ZoneEvent>  zoneEnters;
        std::vector<ZoneEvent>  zoneExits;
        //std::vector<SFX> 

        void Clear()
        {
            shots.clear();
            hits.clear();
            //kills.clear();
            audio.clear();
            zoneEnters.clear();
            zoneExits.clear();
        }

        bool Empty() const
        {
            return shots.empty()
                && hits.empty()
                && audio.empty()
                && zoneEnters.empty()
                && zoneExits.empty();
        }
    };
} // namespace Game
