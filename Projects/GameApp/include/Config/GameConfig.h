#pragma once
#include <string>
#include <cstdint>
#include <vector>

#include <Core/Math/Transform.h>

namespace Murder::Config
{
    struct ModelConfig
    {
        std::string name;
        Transform   localTransform{};
        bool        load = true; // whether to load on register or not
    };

    struct DoorSpawnConfig
    {
        Vector3 position{};
        Vector3 lightOffset{};
        Vector3 axis         = Vector3::Up;
        float   angleRadians = 0.0f;
    };

    struct EnemySpawnConfig
    {
        Vector3     position{};
        std::string model = "skelly.glb";
    };

    struct EncounterConfig
    {
        std::string name;
        std::string levelPart;

        Vector3 zoneCenter{};
        Vector3 zoneExtents{};

        std::vector<EnemySpawnConfig> enemySpawns;
        std::vector<DoorSpawnConfig>  doors;

        uint32_t index = 0;
    };

    struct LevelConfig
    {
        ModelConfig model = { .name = "level01.glb" };

        Vector3 playerSpawnPoint = { 0.0f, 2.0f, 0.0f };

        std::vector<EncounterConfig> encounters;
        std::vector<ModelConfig>     staticGeometry;
    };

    struct PlayerConfig
    {
        Transform spawnTransform{ .position = { 0.0f, 2.0f, 0.0f },
                                  .rotation = Quaternion::CreateFromYawPitchRoll(PI, 0.0f, 0.0f),
                                  .scale    = { 1.0f, 1.0f, 1.0f } };

        Vector3 cameraAttachment = { 0.0f, 1.5f, 0.0f };

        // input
        float moveSpeed        = 10.0f;
        float jumpSpeed        = 5.0f;
        float mouseSensitivity = 1.0f;

        float lookRadiansPerPixel   = 0.001f;
        float lookPitchLimitRadians = PI_DIV2 - 0.001f;

        // health
        float maxHealth = 100.0f;

        // sound
        float       footstepCooldown     = 0.4f;
        float       footstepStartElapsed = 0.0f;
        std::string footstepSoundKey     = "walking";

        // collision
        float characterHeight = 1.0f;
        float characterRadius = 0.25f;

        float deathDuration             = 2.0f;
        float hitInvulnerabilitySeconds = 0.5f;
    };

    struct WeaponConfig
    {
        Game::WeaponType type             = Game::WeaponType::PISTOL;
        float            timeBetweenShots = 0.2f;
        float            damage           = 20.0f;
        float            range            = 100.0f;
    };

    struct DirectionalLightConfig
    {
        Vector3 color{ 1.0f, 1.0f, 1.0f };
        Vector3 direction{ 0.0f, -1.0f, 0.5f };
        float   intensity = 0.5f;
        bool    dynamic   = false;
    };

    struct PointLightConfig
    {
        Vector3 color{ 1.0f, 0.85f, 0.7f };
        Vector3 position{ 0.0f, 3.0f, 0.0f };
        float   intensity = 0.5f;
        float   maxRange  = 7.0f;
    };

    struct SpotLightConfig
    {
        Vector3 color{ 0.75f, 0.85f, 1.0f };
        Vector3 position{ -3.0f, 4.0f, -1.0f };
        Vector3 direction{ 0.25f, -1.0f, 0.1f };
        float   intensity         = 1.4f;
        float   maxRange          = 16.0f;
        float   innerAngleRadians = ToRadians(15.0f);
        float   outerAngleRadians = ToRadians(30.0f);
    };

    struct GameConfig
    {
        LevelConfig  level{};
        PlayerConfig player{};
        WeaponConfig weapon{};
        ModelConfig  viewModel = { .name = "Pistol.glb", .localTransform = { .position = { 0.3f, -.025f, 0.8f } } };

        std::vector<DirectionalLightConfig> directionalLights;
        std::vector<PointLightConfig>       pointLights;
        std::vector<SpotLightConfig>        spotLights;

        void SetDefaults()
        {
            *this = {};
            pointLights.emplace_back(PointLightConfig{});

            {
                pointLights.emplace_back(
                    PointLightConfig{
                        .color     = { 1.0f, 1.0f, 1.0f },
                        .position  = { -3.7f, 1.3f, -6.5f },
                        .intensity = 0.2f,
                        .maxRange  = 32.0f,
                    });

                pointLights.emplace_back(
                    PointLightConfig{
                        .color     = { 0.0f, 0.33f, 0.94f },
                        .position  = { -29.0f, 4.0f, 4.3f },
                        .intensity = 0.7f,
                        .maxRange  = 32.0f,
                    });

                pointLights.emplace_back(
                    PointLightConfig{
                        .color     = { 0.75f, 0.26f, 0.0f },
                        .position  = { -48.6f, 5.4f, 12.4f },
                        .intensity = 0.7f,
                        .maxRange  = 24.0f,
                    });

                pointLights.emplace_back(
                    PointLightConfig{
                        .color     = { 1.0f, 1.0f, 1.0f },
                        .position  = { -25.0f, 3.0f, 4.0f },
                        .intensity = 0.2f,
                        .maxRange  = 10.0f,
                    });

                pointLights.emplace_back(
                    PointLightConfig{
                        .color     = { 1.0f, 0.0f, 0.0f },
                        .position  = { -44.5f, 5.0f, -3.2f },
                        .intensity = 0.5f,
                        .maxRange  = 16.0f,
                    });
                pointLights.emplace_back(
                    PointLightConfig{
                        .color     = { 0.384f, 0.0f, 1.0f },
                        .position  = { -44.5f, 5.0f, -8.2f },
                        .intensity = 0.5f,
                        .maxRange  = 16.0f,
                    });
                pointLights.emplace_back(
                    PointLightConfig{
                        .color     = { 1.0f, 0.25f, 0.25f },
                        .position  = { -38.0f, 5.0f, 0 },
                        .intensity = 1.0f,
                        .maxRange  = 20.0f,
                    });
                pointLights.emplace_back(
                    PointLightConfig{
                        .color     = { 0.321f, 0.04f, 0.04f },
                        .position  = { -43.0f, -2.0f, 44.0f },
                        .intensity = 1.0f,
                        .maxRange  = 20.0f,
                    });
                pointLights.emplace_back(
                    PointLightConfig{
                        .color     = { 0.321f, 0.04f, 0.04f },
                        .position  = { -43.0f, -2.0f, 56.0f },
                        .intensity = 1.0f,
                        .maxRange  = 20.0f,
                    });
            }

            spotLights.emplace_back(SpotLightConfig{});

            {
                spotLights.emplace_back(
                    SpotLightConfig{ .color             = { 1.0f, 0.64f, 0.643f },
                                     .position          = {},
                                     .direction         = {},
                                     .intensity         = 0.5f,
                                     .maxRange          = 20,
                                     .innerAngleRadians = ToRadians(9.3f),
                                     .outerAngleRadians = 1.062f });

                spotLights.emplace_back(
                    SpotLightConfig{ .color             = { 1.0f, 1.0f, 1.0f },
                                     .position          = { -50.0f, 50.0f, -200.0f },
                                     .direction         = { 0, -1.0f, -1.0f },
                                     .intensity         = 10.0f,
                                     .maxRange          = 1024.0f,
                                     .innerAngleRadians = ToRadians(15),
                                     .outerAngleRadians = ToRadians(90.00f) });
            }
        }

        // TODO: replace with these
        // no-op for now
        bool LoadFromFile(std::string_view path) {}
        bool SaveToFile(std::string_view path) const {}
    };

} // namespace Murder::Config
