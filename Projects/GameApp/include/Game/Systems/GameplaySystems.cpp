#include "Systems.h"

#include "SystemCommon.h"

#include <Utility/Logging.h>

#define LOG_TAG "GameplaySystems"

namespace Game
{
    namespace
    {
        // combo
        ComboRank CalculateRankUp(Combo& combo, float score)
        {
            int current = static_cast<int>(combo.rank);

            for (int i = current + 1; i <= static_cast<int>(ComboRank::SPlus); ++i)
                if (score >= ComboTable[i].threshold)
                    combo.rank = static_cast<ComboRank>(i);
            return combo.rank; // Potentially don't need this
        }

        void AddComboScore(Combo& combo, float amount)
        {
            combo.score += amount;

            auto maxScore = ComboTable[static_cast<int>(ComboRank::SPlus)].threshold;

            if (combo.rank == ComboRank::SPlus and combo.score > maxScore)
                combo.score = maxScore;
            combo.totalScore += amount;

            CalculateRankUp(combo, combo.score);
        }
    } // namespace
    /******************************************************************************************************************
     * STATIC GAMEPLAY SYSTEMS
     * 
     * These will never go away until the game exists.
     *****************************************************************************************************************/

    // System only used for updating weapon cooldown (for now)
    bool WeaponUpdateSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!PlayerControlActive(ecs))
            return false;

        for (auto [player, weapon] : ecs.View<PlayerTag, Weapon>().each())
            weapon.weaponCooldown -= (weapon.weaponCooldown > 1e-6) ? dt : 0;
        return false;
    }

    bool EnemyTargetSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        for (auto [enemy, target, enemyTransform, velocity] : ecs.View<EnemyTag, Target, TransformC, Velocity>().each())
        {
            if (target.target == entt::null)
            {
                for (auto [player, playerTransform] : ecs.View<PlayerTag, TransformC>().each())
                {
                    Vector3 delta = playerTransform.position - enemyTransform.position;

                    // TODO: Store attribute
                    constexpr float detectionRangeSqr = 100.0f;
                    if (delta.LengthSquared() < detectionRangeSqr)
                    {
                        target.target = player;
                        break;
                    }
                }
            }

            if (target.target == entt::null)
                continue;

            if (auto playerTransform = ecs.TryGet<TransformC>(target.target); playerTransform)
                if (auto attachment = ecs.TryGet<CameraAttachment>(target.target); attachment)
                {
                    Vector3 delta = playerTransform->position + attachment->offset - enemyTransform.position;
                    Vector3 dir   = delta;
                    dir.Normalize();

                    // TODO: Store attribute
                    constexpr float speed = 7.0f;
                    velocity              = dir * speed;

                    Vector3 forward = -dir; // I think our enemy model might be flipped :)
                    Vector3 up      = Vector3::Up;
                    Vector3 right   = up.Cross(forward);
                    right.Normalize();
                    up = forward.Cross(right);

                    enemyTransform.rotation = Quaternion::CreateFromRotationMatrix(Matrix(right, up, forward));

                    // TODO: Move and use actual collision detection
                    constexpr float attackRangeSqr = 2.0f;
                    constexpr float attackDamage   = 10.0f;
                    if (delta.LengthSquared() <= attackRangeSqr)
                    {
                        if (auto health = ecs.TryGet<Health>(target.target))
                        {
                            if (auto invulnerability = ecs.TryGet<Invulnerability>(target.target))
                                continue;

                            health->hp -= attackDamage;
                            ecs.Emplace<Invulnerability>(target.target, 0.5f);
                        }
                    }
                }
        }
        return false;
    }

    bool InvulnerabilitySystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        for (auto [entity, invulnerability] : ecs.View<Invulnerability>().each())
        {
            invulnerability.time -= dt;

            if (invulnerability.time <= 0.0f)
                ecs.Remove<Invulnerability>(entity);
        }
        return false;
    }

    bool WobbleSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        for (auto [entity, wobble, model] : ecs.View<Wobble, ModelComponent>().each())
        {
            wobble.timeElapsed += dt;

            float intensity = wobble.intensity * std::max(1.0f - wobble.timeElapsed / wobble.duration, 0.0f);
            float radAngle  = ToRadians(wobble.timeElapsed * wobble.speed * 360.0f);

            model.transform.rotation = Quaternion::CreateFromYawPitchRoll(
                cosf(radAngle) * intensity,
                0.0f,
                sinf(radAngle) * intensity
            );

            if (wobble.timeElapsed >= wobble.duration)
                ecs.Remove<Wobble>(entity);
        }
        return false;
    }

    bool Game::HitEnemySystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        auto& events = ecs.GetOrEmplaceContext<FrameEvents>();

        for (auto& hit : events.hits)
        {
            if (ecs.Has<DeadTag>(hit.target))
                continue;

            if (auto wobble = ecs.TryGet<Wobble>(hit.target))
            {
                wobble->duration  += 1.0f;
                wobble->intensity += 0.25f;
            }
            else
            {
                ecs.Emplace<Wobble>(hit.target, Wobble{ .duration = 1.0f, .intensity = 0.25f });
            }

            if (auto* health = ecs.TryGet<Health>(hit.target))
            {
                health->hp -= hit.damage;
                LOG_DEBUG("Enemy health: " << health->hp << "/" << health->maxHp);
                if (health->hp <= 0.0f)
                {
                    auto playerEntity = ecs.FindFirst<PlayerTag>();
                    auto combo        = ecs.TryGet<Combo>(playerEntity);

                    const auto& data = ComboTable[static_cast<int>(combo->rank)];

                    auto state = ecs.TryGet<PlayerState>(playerEntity);
                    if (!state->grounded)
                        AddComboScore(*combo, 30.0f * data.bonus);
                    else
                        AddComboScore(*combo, 10.0f * data.bonus);

                    combo->killCount++;
                    ecs.EmplaceOrReplace<DeadTag>(hit.target);
                    LOG_DEBUG("Enemy Killed: " << static_cast<uint32_t>(hit.target));
                    LOG_DEBUG("Kill count: " << static_cast<uint32_t>(combo->killCount));
                    events.audio.push_back({ "monsterDeath" });
                }
                else
                {
                    for (auto [entity, combo] : ecs.View<PlayerTag, Combo>().each())
                    {
                        const auto& data = ComboTable[static_cast<int>(combo.rank)];
                        AddComboScore(combo, 5.0f * data.bonus);
                    }
                }

                events.audio.push_back({ "damage" });
            }
        }
        events.hits.clear();

        return false;
    }

    bool DeadEnemySystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        for (auto [entity] : ecs.View<DeadTag>().each())
        {
            if (RigidBody* body = ecs.TryGet<RigidBody>(entity))
                ecs.ContextRef<Physics>().DestroyBody(*body);

            if (auto encounterEnemy = ecs.TryGet<EncounterEnemy>(entity))
            {
                if (auto encounter = ecs.TryGet<Encounter>(encounterEnemy->encounter))
                    encounter->enemyCount--;
            }

            ecs.Destroy(entity);
        }
        return false;
    }

    bool ZoneDetectionSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        FrameEvents& events = ecs.GetOrEmplaceContext<FrameEvents>();

        events.zoneEnters.clear();
        events.zoneExits.clear();

        for (auto [entity, transform, zoneTracker] : ecs.View<TransformC, ZoneTracker>().each())
        {
            Entity currentZone = zoneTracker.current;
            Entity newZone     = entt::null;

            // Check against current zone
            if (currentZone != entt::null)
            {
                ZoneBoundary* boundary = ecs.TryGet<ZoneBoundary>(currentZone);

                if (boundary && boundary->box.Contains(transform.position))
                    continue;
            }

            // Check for new zone
            for (auto [zone, boundary] : ecs.View<ZoneBoundary>().each())
            {
                if (boundary.box.Contains(transform.position))
                {
                    newZone = zone;
                    break;
                }
            }

            if (currentZone != newZone)
            {
                zoneTracker.last    = currentZone;
                zoneTracker.current = newZone;

                // Check for exit
                if (currentZone != entt::null)
                    events.zoneExits.emplace_back(ZoneEvent{ .zone = currentZone, .entity = entity });

                // Check for enter
                if (newZone != entt::null)
                    events.zoneEnters.emplace_back(ZoneEvent{ .zone = newZone, .entity = entity });
            }
        }
        return false;
    }

    bool ZoneEnterSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        FrameEvents& events = ecs.GetOrEmplaceContext<FrameEvents>();

        for (auto& event : events.zoneEnters)
        {
            LOG_DEBUG("Triggered zone " << static_cast<std::uint32_t>(event.zone));

            // Zones are not guaranteed to be encounters
            if (auto encounter = ecs.TryGet<Encounter>(event.zone); encounter && !encounter->triggered)
                encounter->triggered = true;

            // TODO: Add end zone
            if (auto winZone = ecs.TryGet<WinZone>(event.zone); winZone && !winZone->triggered)
            {
                winZone->triggered = true;
                auto view = ecs.View<PlayerTag, PlayerState, Health>().each();
                for (auto [entity, state, health] : view)
                {
                    health.hp = 0.0f;
                }
            }
        }
        return false;
    }

    bool EncounterUpdateSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        for (auto [encounterEntity, encounter] : ecs.View<Encounter>().each())
        {
            if (encounter.triggered && encounter.wave < 0)
            {
                encounter.active = true;
                AddEncounterDoors(ecs, encounterEntity);
                LOG_DEBUG("Started encounter " << static_cast<std::uint32_t>(encounterEntity));
            }

            if (!encounter.active)
                continue;

            int wave     = encounter.wave;
            int nextWave = wave;

            if (wave < 0)
                nextWave = 0; // Start first wave
            else if (encounter.enemyCount == 0 && encounter.waveTimeElapsed >= encounter.waveDelay)
                nextWave = wave + 1; // Start next wave

            encounter.waveTimeElapsed += dt;

            if (nextWave > wave)
            {
                encounter.wave            = nextWave;
                encounter.waveTimeElapsed = 0.0f;
            }

            if (encounter.enemyCount == 0 && encounter.waveTimeElapsed >= encounter.waveDelay)
            {
                for (auto [entity, spawn, transform] : ecs.View<EnemySpawn, TransformC>().each())
                {
                    if (spawn.encounter == encounterEntity && spawn.wave == encounter.wave)
                    {
                        Entity enemy = AddEnemy(
                            ecs,
                            "skelly.glb",
                            transform,
                            {},
                            encounterEntity);
                        ecs.Get<Target>(enemy).target = ecs.FindFirst<PlayerTag>(); // I don't like this
                        encounter.enemyCount++;
                    }
                }

                if (encounter.enemyCount == 0)
                {
                    encounter.active = false;
                    RemoveEncounterDoors(ecs, encounterEntity);
                    LOG_DEBUG("Finished encounter " << static_cast<std::uint32_t>(encounterEntity));
                }
            }
        }
        return false;
    }

    bool ComboDrainSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        for (auto [entity, combo] : ecs.View<PlayerTag, Combo>().each())
        {
            int rankIndex = static_cast<int>(combo.rank);

            if (rankIndex == 0) // check if no combo
                continue;

            const auto& data = ComboTable[static_cast<int>(combo.rank)];

            combo.score -= data.decay * dt; // decrease current combo score

            // check if combo is fully depleted
            if (combo.score <= 1e-6)
            {
                rankIndex--;

                combo.rank  = static_cast<ComboRank>(rankIndex); // lowers the combo rank
                combo.score = ComboTable[rankIndex].threshold;   // sets current combo to previous threshold
            }
        }
        return false;
    }

    bool ComboSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        auto& cfg = ecs.Context<Config::GameConfig>();

        for (auto [entity, combo, params] : ecs.View<PlayerTag, Combo, PlayerParams>().each())
        {
            if (combo.rank != ComboRank::None)
            {
                const auto& data = ComboTable[static_cast<int>(combo.rank)];
                params.moveSpeed = cfg.player.moveSpeed * data.bonus;
            }
        }
        return false;
    }

    /******************************************************************************************************************
     * DYNAMIC SYSTEMS -- These go away after they return true
     *****************************************************************************************************************/

    bool ShotsFiredSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (!GameRunning(ecs))
            return false;

        Physics& physics = ecs.ContextRef<Physics>();
        auto&    events  = ecs.GetOrEmplaceContext<FrameEvents>();

        // no shots
        if (events.shots.empty())
            return false; // keep system alive

        for (auto& shot : events.shots)
        {
            auto weapon = ecs.TryGet<Weapon>(shot.shooter);

            if (!weapon)
                continue;

            if (weapon->weaponCooldown > 1e-6) // only shoot when allowed to
            {
                LOG_DEBUG("Weapon not of cooldown. Time left: " << weapon->weaponCooldown);
                continue;
            }

            if (weapon->weaponCooldown <= 1e-6) // if shot, set cooldown
                weapon->weaponCooldown = weapon->timeBetweenShots;

            events.audio.push_back({ "gunshot" });

            Vector3 rOrigin = shot.origin;
            Vector3 rDir    = shot.dir;
            rDir.Normalize();

            RayResult result;
            bool      hit = physics.RayCast(rOrigin, rDir, weapon->range, result);

            Entity hitEntity = entt::null;

            if (hit)
            {
                for (auto [entity, body] : ecs.View<RigidBody, EnemyTag>().each())
                {
                    if (result.hitBody == body.id)
                    {
                        hitEntity = entity;
                        break;
                    }
                }
            }

            // check if we did a hit
            if (hitEntity != entt::null)
            {
                LOG_DEBUG("HIT enemy " << static_cast<uint32_t>(hitEntity) << " dist: " << result.distance);
                ecs.EmplaceOrReplace<HitTag>(hitEntity);
                events.hits.push_back({ hitEntity, weapon->damage });
            }
            else
            {
                LOG_DEBUG("Miss!");
            }
        }
        events.shots.clear();
        return false;
    }

} // namespace Game
