/**
 * @file SystemCommon.h
 *
 * @brief Helper functions for systems. For functions shared across all systems
 */
#pragma once

#include <ECS/ECSManager.h>
#include <AssetManager.h>
#include <Input.h>

// included for global presence across all systems
#include "Game/State/GameState.h"
#include "Game/State/PlayerState.h"
#include "Game/Components.h"
#include "Common/Scene/SceneComponents.h"
#include "Config/GameConfig.h"
#include "Game/Events/Events.h"

#include "Game/DebugSettings.h"

// debug includes
#ifdef _DEBUG
#include "DebugRenderer.h"
#ifdef USING_IMGUI
#include <imgui.h>
#endif #endif
#endif

namespace Game
{
    using namespace ECS;
    using ECS::ECSManager;

    // -- Game State Helpers --
    inline bool GameRunning(ECSManager& ecs) { return ecs.ContextRef<GameState>().IsRunning(); }
    inline bool GamePaused(ECSManager& ecs) { return ecs.ContextRef<GameState>().IsPaused(); }
    inline bool Editing(ECSManager& ecs) { return ecs.ContextRef<GameState>().IsEditing(); }
    inline bool GameOver(ECSManager& ecs) { return ecs.ContextRef<GameState>().IsEnded(); }
    inline bool PlayerControlActive(ECSManager& ecs) { return GameRunning(ecs) || Editing(ecs); }

    // -- Global Helper functions, move as needed --

    inline std::optional<AABB> TryGetWorldAABB(AssetManager& assets, ModelId modelId, const TransformC& transform)
    {
        // early out if invalid model
        if (!modelId || !assets.LoadModel(modelId))
            return std::nullopt;

        // early out if model does not exist or has no bounds
        const auto* model = assets.TryGetModel(modelId);
        if (!model || !model->bounds.has_value() || !model->bounds->IsValid())
            return std::nullopt;

        // model bounds are local
        const AABB& local = *model->bounds;

        const Vector3 lCenter = DirectX::XMVector3Transform(local.Center(), TransformMath::World(transform));

        auto temp              = transform;
        temp.position          = Vector3{ 0.0f, 0.0f, 0.0f };
        const Vector3 lExtents = XMVector3Transform(local.Extents(), TransformMath::World(temp));

        return AABBFromCenterExtents(lCenter, lExtents);
    }

    inline RigidBody CreateBoxBody(
        ECSManager&       ecs,
        AssetManager&     assets,
        ModelId           modelId,
        const TransformC& transform,
        const Vector3&    fallbackExtents,
        BodyType          bType = BodyType::DYNAMIC)
    {
        Physics& physics = ecs.ContextRef<Physics>();

        Vector3 center  = transform.position;
        Vector3 extents = fallbackExtents;

        if (auto bounds = TryGetWorldAABB(assets, modelId, transform))
        {
            center  = bounds->Center();
            extents = bounds->Extents();
        }

        return RigidBody(physics.CreateBox(center, Quaternion::Identity, extents, bType));
    }

    inline Entity AddEnemy(
        ECSManager&        ecs,
        const std::string& model          = "skull.glb",
        const TransformC&  transform      = {},
        const TransformC&  localTransform = {},
        Entity             encounter      = entt::null)
    {
        auto& assets = ecs.ContextRef<AssetManager>();

        auto ent = ecs.Create();
        ecs.Emplace<EnemyTag>(ent);
        ecs.Emplace<SpawnTag>(ent);
        ecs.Emplace<Health>(ent, 40.0f);
        ecs.Emplace<TransformC>(ent, transform);
        ecs.Emplace<Velocity>(ent);
        ecs.Emplace<Target>(ent);

        if (encounter != entt::null)
            ecs.Emplace<EncounterEnemy>(ent, encounter);

        auto modelC = ecs.Emplace<ModelComponent>(
            ent,
            ModelComponent{ .id = assets.RegisterModel(model), .transform = localTransform });

        auto body =
            ecs.Emplace<RigidBody>(ent, CreateBoxBody(ecs, assets, modelC.id, transform, Vector3{ 0.5f, 0.5f, 0.5f }));

        ecs.Emplace<RecurringSound>(ent, 8.0f, 0.0f, "EnemySound");
        ecs.ContextRef<Physics>().SetBodyGravityFactor(body, 0.0f);

        return ent;
    }

    /// @brief Load a model from ModelConfig
    /// @param assets Asset manager
    /// @param cfg model config
    /// @return true if succeed, false if not
    inline ModelComponent CreateModelComponentFromConfig(AssetManager& assets, Config::ModelConfig& cfg)
    {
        ModelComponent out{};
        out.id        = assets.RegisterModel(cfg.name);
        out.transform = cfg.localTransform;
        // TODO: handle overrides

        if (cfg.load)
            assets.LoadModel(out.id);

        return out;
    }

    inline Entity AddModelComponent(ECSManager& ecs, std::string_view modelPath, const TransformC& transform)
    {
        auto& assets = ecs.ContextRef<AssetManager>();

        ModelComponent model{ .id = assets.RegisterModel(modelPath), .transform = {} };

        const Entity e = ecs.Create();
        ecs.Emplace<TransformC>(e, transform);
        ecs.Emplace<ModelComponent>(e, std::move(model));

        return e;
    }

    inline void AddEncounterDoors(ECSManager& ecs, Entity encounter = entt::null)
    {
        auto&    assets  = ecs.ContextRef<Murder::AssetManager>();
        Physics& physics = ecs.ContextRef<Physics>();
        ModelId  modelId = assets.RegisterModel("forcefield.glb");

        assets.LoadModel(modelId);

        for (auto [entity, spawn, transform] : ecs.View<Game::DoorSpawn, Game::TransformC>().each())
        {
            if (spawn.encounter == encounter)
            {
                auto door = ecs.Create();
                ecs.Emplace<Game::Door>(door, encounter);
                ecs.Emplace<Game::TransformC>(door, transform);
                ecs.Emplace<Murder::ModelComponent>(door, Murder::ModelComponent{ .id = modelId, .transform = {} });

                if (const auto model = assets.TryGetModel(modelId))
                {
                    for (auto& part : model->parts)
                    {
                        if (const auto mesh = assets.TryGetMesh(part.meshId))
                        {
                            ecs.Emplace<Game::RigidBody>(
                                door,
                                physics.CreateMesh(transform.position, mesh->positions, mesh->indices));
                        }
                    }
                }

                Vector3 localLightOffset =
                    Vector3::Transform(spawn.lightOffset, Matrix::CreateFromQuaternion(transform.rotation));
                ecs.Emplace<PointLight>(
                    door,
                    PointLight{
                        .color     = Vector3{ 0.5f, 0.5f, 1.0f },
                        .position  = transform.position + localLightOffset,
                        .intensity = 1.0f,
                        .maxRange  = 8.0f,
                    });
            }
        }
    }

    inline void RemoveEncounterDoors(ECSManager& ecs, Entity encounter)
    {
        Physics& physics = ecs.ContextRef<Physics>();

        for (auto [entity, door, body] : ecs.View<Game::Door, Game::RigidBody>().each())
        {
            if (door.encounter == encounter)
            {
                physics.DestroyBody(body);
                ecs.Destroy(entity);
            }
        }
    }
} // namespace Game
