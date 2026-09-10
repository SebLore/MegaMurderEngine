#pragma once

#include <ECS/ISystem.h>

// forward declaration to avoid including the entire ECSManager
namespace ECS
{
    class ECSManager;
}

namespace Game
{
    // aliases for convenience
    using ECS::ECSManager;
    using ECS::ISystem;
    using ECS::SystemType;

    // -- Priority Constants --
    namespace Priority
    {
        inline constexpr int STARTUP = 1000;  // Run before all other systems, and remove after running once
        inline constexpr int FIRST   = 900;   // Run before all other priorities
        inline constexpr int DEFAULT = 0;     // Default
        inline constexpr int LAST    = -1000; // System should run after all other systems
    } // namespace Priority

    /**************************************************************************
     * Base policy class for typed systems -- use CRTP to avoid virtual GetType and Priority
     * https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
     **************************************************************************/
    template <SystemType TypeValue, int PriorityValue = 0>
    class TypedSystem : public ISystem
    {
      public:
        int        Priority() const override { return PriorityValue; }
        SystemType GetType() const final { return TypeValue; }
    };

    // run once at startup, then removed
    using StartupSystem = TypedSystem<SystemType::Dynamic, Priority::STARTUP>;

    // live for the entire program, cannot be removed
    using StaticSystem = TypedSystem<SystemType::Static>;

    // live until they return true from OnUpdate, then removed
    using DynamicSystem = TypedSystem<SystemType::Dynamic>; // this is the default, just here for clarity

    // TODO: define system policies for systems that should collect data, use data, pre-render operations etc.

    /*****************************************************************************
     * START-UP SYSTEM -- One and done
     *****************************************************************************/
    /// Loads the game state
    class LoadSystem : public StartupSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) final;
    };

    class WorldCreateSystem : public StartupSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) final;
    };

    /// Initializes the player
    class PlayerInitializeSystem : public StartupSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) final;
    };

    class AudioInitializeSystem : public StartupSystem
    {
      public:
        bool OnUpdate(ECSManager&, float) final;
    };

    class HUDInitializeSystem : public StartupSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) final;
    };

    /// Emplace all the runtime systems. Add this last
    class RuntimeInitializeSystem : public StartupSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) final;
    };

    /*****************************************************************************
     * STATIC SYSTEMS -- Live for the entire program
     *****************************************************************************/
    class GameInputModeSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class GamePauseSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class GameOverSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    // -- Player Systems --
    class PlayerInputSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class PlayerUpdateSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class PlayerStateSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class PlayerDeathSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class WeaponUpdateSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class ComboDrainSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class ComboSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class SoundEventSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) final;
    };

    /// Checks for input
    class InputFocusSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class EnemyTargetSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class InvulnerabilitySystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class WobbleSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class HitEnemySystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class DeadEnemySystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class ZoneDetectionSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class ZoneEnterSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class EncounterUpdateSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class AudioSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class EnemyAudioSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };
    /*****************************************************************************
     * GUI SYSTEMS
     *****************************************************************************/
    class UpdateHudSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    /*****************************************************************************
     * PHYSICS SYSTEMS
     *****************************************************************************/
    class PhysicsPreUpdateSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class PhysicsUpdateSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class PhysicsPostUpdateSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    /*****************************************************************************
     * PRE-RENDER PASS SYSTEMS
     *****************************************************************************/

    class BuildRenderWorldSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) final;
        int  Priority() const override { return Priority::LAST; }
    };

    class GameOverlaySystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    /*****************************************************************************
     * DEBUG SYSTEMS
     *****************************************************************************/
#ifdef _DEBUG
    class GameEditSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class DebugGuiStartSystem : public StaticSystem
    {
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class DebugSpawnEntitySystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class DebugDrawZonesSystem : public StaticSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class DebugLevelEditorSystem : public StaticSystem
    {
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class DebugGuiEndSystem : public StaticSystem
    {
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };
#endif
    /*****************************************************************************
     * DYNAMIC SYSTEMS -- Die after they are done
     *****************************************************************************/
    class ShotsFiredSystem : public DynamicSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class GameRestartDelaySystem : public DynamicSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class GameResetSystem : public DynamicSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

    class MainMenuSystem : public DynamicSystem
    {
      public:
        bool OnUpdate(ECSManager& ecs, float dt) override;
    };

} // namespace Game
