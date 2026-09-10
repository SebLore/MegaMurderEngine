/**
 * @file Components.h
 * @brief Storage for basic ECS Components. Later could be collective header for different types
 */
#pragma once

#include <Input.h>

#include <ECS.hpp>

#include <Core/Base/Common.h>
#include <Core/Math/Transform.h>
#include <Common/Scene/SceneComponents.h>

#include "SpriteText.h"
#include "Engine/Physics/Physics.h"

namespace Game
{
    using namespace Murder;
    using Murder::Transform;

    // Tags
    // clang-format off
    struct PlayerTag{};
    struct EnemyTag{};
    struct LevelTag{};
    struct GunTag{};
    struct HitTag{};
    struct DeadTag{};
    struct SpawnTag{};
    struct WeaponTag {};
    struct MainCameraTag{};


    // gui tags
    struct PauseOverlayTag{};
    struct EditOverlayTag{};
    struct GameOverOverlayTag{};
    struct MainMenuTag{};


    struct ViewModelTag{};
    // clang-format on

    struct CameraAttachment
    {
        Vector3 offset;
    };

    // input
    struct InputMode
    {
        bool uiMode           = true; // disable game input if true
        bool appFocused       = true;
        bool suppressLookOnce = false; // prevent big jump of the camera after ui mode
    };

    struct UiViewport
    {
        float width      = 1024.0f;
        float height     = 768.0f;
        float baseWidth  = 1024.0f;
        float baseHeight = 768.0f;
        float scale      = 1.0f;

        void SetSize(float w, float h)
        {
            width  = std::max(1.0f, w);
            height = std::max(1.0f, h);
            scale  = std::max(0.5f, std::min(width / baseWidth, height / baseHeight));
        }
    };

    struct RecurringSound
    {
        float cooldown = 0.4f;
        float elapsed  = 0.0f;

        std::string key;
    };

    // Render/Drawing data

    // Gun viewmodel
    struct GunViewModel
    {
        Vector3 offset    = { 0.3f, -0.25f, 0.8f }; // in camera space
        float   recoil    = 0.0f;                   // current recoil amount
        float   recoilVel = 0.0f;                   // spring velocity
    };

    using TransformC = Transform;
    using Velocity   = Vector3;

    // Zones

    struct ZoneBoundary
    {
        DirectX::BoundingBox box{};

        explicit ZoneBoundary(DirectX::BoundingBox box) : box(box) {}
        ZoneBoundary(Vector3 center, Vector3 extents) : box(center, extents) {}

        operator DirectX::BoundingBox() const { return box; }
    };

    struct ZoneTracker
    {
        ECS::Entity current = entt::null;
        ECS::Entity last    = entt::null;
    };

    // end zone tag
    struct WinZone
    {
        bool triggered = false;
    };

    // Encounters

    struct Encounter
    {
        bool  triggered       = false;
        bool  active          = false;
        int   enemyCount      = 0;
        int   wave            = -1;
        float waveDelay       = 1.0f; // In seconds
        float waveTimeElapsed = 0.0f;
    };

    struct EncounterPart
    {
        ECS::Entity encounter = entt::null;
    };

    // Could also contain enemy type (if we had more than one type)
    struct EnemySpawn : public EncounterPart
    {
        int wave = 0;
    };

    struct EncounterEnemy : public EncounterPart
    {
    };

    struct DoorSpawn : public EncounterPart
    {
        Vector3 lightOffset{};
    };

    struct Door : public EncounterPart
    {
    };

    // Enemy

    struct Target
    {
        ECS::Entity target = entt::null;
    };

    // Input
    struct Controls
    {
        Input::Control forward{};
        Input::Control backward{};
        Input::Control left{};
        Input::Control right{};
        Input::Control jump{};
        Input::Control fire{};
    };

    struct CameraLook
    {
        float yaw   = 0.0f; // radians
        float pitch = 0.0f; // radians
    };

    // stay the same
    struct PlayerParams
    {
        float moveSpeed        = 10.0f; // units/s
        float jumpSpeed        = 5.0f;  // units/s
        float mouseSensitivity = 1.0f;  // aka sensitivity
    };

    // inputs taken this frame, reset every frame
    struct PlayerInput
    {
        int moveForward = 0;
        int moveRight   = 0;

        float lookDX = 0.0f;
        float lookDY = 0.0f;

        bool moved  = false;
        bool turned = false;

        bool fired      = false;
        bool jump       = false;
        bool swapWeapon = false;
    };

    // Combat data
    struct Health
    {
        float hp    = 100.0f;
        float maxHp = 100.0f;

        Health(float hp, float maxHp) : hp(hp), maxHp(maxHp) {}
        Health(float hp) : Health(hp, hp) {}
    };

    struct Invulnerability
    {
        float time = 0.0f;
    };

    struct Wobble
    {
        float duration    = 1.0f; // In seconds
        float speed       = 1.0f; // In revolutions per second
        float intensity   = 1.0f;
        float timeElapsed = 0.0f;
    };

    enum class WeaponType : uint8_t
    {
        PISTOL,
        SHOTGUN
    };

    struct Weapon
    {
        WeaponType type             = WeaponType::PISTOL;
        float      cooldown         = 0.0f; // for alt fire etc.
        float      timeBetweenShots = 0.5f; // max time between shots
        float      weaponCooldown   = 0.0f; // time left until weapon can be shot again
        float      damage           = 20.0f;
        float      range            = 100.0f; // how far to do the ray cast
    };

    // Combo
    enum class ComboRank : uint8_t
    {
        None,
        D,
        C,
        B,
        A,
        S,
        SPlus
    };

    struct Combo
    {
        float score      = 0.0f;
        float totalScore = 0.0f;

        ComboRank rank = ComboRank::None;

        // just testing variables
        bool      broken    = false;
        int       killCount = 0;
        ComboRank maxRank   = ComboRank::None;
    };

    struct Cooldown
    {
        float duration = 2.0f;
        float elapsed  = 0.0f;

        void Update(float dt) { elapsed += dt; }
        bool Done() const { return elapsed >= duration; }
        void Reset() { elapsed = 0.0f; }
    };

    struct DeathTimer : Cooldown
    {
    };
    struct RestartDelay : Cooldown
    {
    };

    struct ComboRankData
    {
        float threshold;
        float decay;
        float bonus;
    };

    static const ComboRankData ComboTable[] = {
        { 0.0f, 0.0f, 1.0f },            // None
        { 50.0f, 5.0f * 2.0f, 1.1f },    // D
        { 60.0f, 6.667f * 2.0f, 1.125 }, // C
        { 70.0f, 8.0f * 2.0f, 1.5f },    // B
        { 80.0f, 10.0f * 2.0f, 1.75f },  // A
        { 90.0f, 15.0f * 2.0f, 2.0f },   // S
        { 110.0f, 20.0f * 2.0f, 2.5f }   // S+
    };

    // Physics

    struct RigidBody
    {
        const PhysicsID id;

        explicit RigidBody(PhysicsID id) : id(id) {}

        operator PhysicsID() const { return id; }
    };

    struct CharacterController
    {
        const PhysicsID id;

        explicit CharacterController(PhysicsID id) : id(id) {}

        operator PhysicsID() const { return id; }
    };

    // menu

    struct MenuButton
    {
        Vector2 scale;
        Vector2 position;
        Sprite  sprite;
    };

    struct MenuComponent
    {
        Sprite                  background;
        std::vector<MenuButton> buttons;
    };

    struct TextComponent
    {
        SpriteText              points;
        SpriteText              buttons;
        std::vector<SpriteText> textLines;
    };

} // namespace Game
