#pragma once

#include <ECS.hpp>

namespace Game
{
    /// Add static systems
    void Configure(ECS::ECSManager& ecs);

    /// Spawn entities, alt. add LoadSystem that spawns them
    void Load(ECS::ECSManager& ecs);
} // namespace Game
