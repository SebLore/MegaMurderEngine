#include "Systems.h"

#include "SystemCommon.h"

#include "Game/Components.h"

#include <ECS/ECSManager.h>

#include "Engine/Physics/Physics.h"

#include <Utility/Logging.h>

#define LOG_TAG "PhysicsSystems"

using namespace DirectX::SimpleMath;
using namespace ECS;

bool Game::PhysicsPreUpdateSystem::OnUpdate(ECSManager& ecs, float dt)
{
    if (!PlayerControlActive(ecs))
        return false;

    Physics& physics = ecs.ContextRef<Physics>();

    // Sync characters
    for (auto [entity, character, transform] : ecs.View<CharacterController, TransformC>().each())
    {
        //if (transform.dirty)
        {
            physics.SetCharacterTransform(character, transform);
        }

        if (Velocity* velocity = ecs.TryGet<Velocity>(entity))
            physics.SetCharacterVelocity(character, *velocity);
    }

    // Sync bodies
    for (auto [entity, body, transform] : ecs.View<RigidBody, TransformC>().each())
    {
        //if (transform.dirty)
        {
            physics.SetBodyTransform(body, transform);
        }

        if (Velocity* velocity = ecs.TryGet<Velocity>(entity))
            physics.SetBodyVelocity(body, *velocity);
    }
    return false;
}

bool Game::PhysicsUpdateSystem::OnUpdate(ECSManager& ecs, float dt)
{
    if (!PlayerControlActive(ecs))
        return false;

    Physics& physics = ecs.ContextRef<Physics>();
    physics.Update(dt);
    return false;
}

bool Game::PhysicsPostUpdateSystem::OnUpdate(ECSManager& ecs, float dt)
{
    if (!PlayerControlActive(ecs))
        return false;

    Physics& physics = ecs.ContextRef<Physics>();

    // Sync characters
    for (auto [entity, character, transform] : ecs.View<CharacterController, TransformC>().each())
    {
        physics.GetCharacterTransform(character, transform);

        if (Velocity* velocity = ecs.TryGet<Velocity>(entity))
            physics.GetCharacterVelocity(character, *velocity);
    }

    // Sync bodies
    for (auto [entity, body, transform] : ecs.View<RigidBody, TransformC>().each())
    {
        physics.GetBodyTransform(body, transform);

        if (Velocity* velocity = ecs.TryGet<Velocity>(entity))
            physics.GetBodyVelocity(body, *velocity);
    }
    return false;
}

#undef LOG_TAG
