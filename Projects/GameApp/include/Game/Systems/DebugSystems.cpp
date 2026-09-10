#include "Systems.h"

#include "SystemCommon.h"

#include "Common/Scene/RenderWorld.h"
#include "Game/DebugSettings.h"
#include "Utility/RNG.h"
#include <Utility/Logging.h>

#define LOG_TAG "DebugSystems"

using namespace ECS;
using namespace Murder;
using namespace DirectX;

#ifdef _DEBUG

namespace Game
{
    namespace
    {
        /// @brief  return a transformc struct with position contained in the bounds
        TransformC RandomPosTransformC(Vector3 center, Vector3 extents)
        {
            float x = RNG::SingleFloatRange(center.x - extents.x, center.x + extents.x);
            float y = RNG::SingleFloatRange(center.y - extents.y, center.y + extents.y);
            float z = RNG::SingleFloatRange(center.z - extents.z, center.z + extents.z);

            return TransformC{ .position = { x, y, z } };
        }

    } // namespace

    /*****************************************************************************
    * GUI SYSTEMS
    *****************************************************************************/

    bool GameEditSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        auto& state = ecs.ContextRef<GameState>();
        auto& input = ecs.ContextRef<Input>();

        if (input.IsPressed(DirectX::Keyboard::Keys::OemPipe) || input.IsPressed(DirectX::Keyboard::Keys::F1))
        {
            if (state.IsEditing())
            {
                LOG_DEBUG("Exiting Edit mode...");
                state.Run();
            }
            else if (state.IsRunning() || state.IsPaused() /*|| state.IsMainMenu()*/)
            {
                LOG_DEBUG("Entering Edit mode...");
                state.Edit();
            }
        }

        return false;
    }

    bool DebugSpawnEntitySystem::OnUpdate(ECSManager& ecs, float dt)
    {
        Input& input = ecs.ContextRef<Input>();

        bool spawnFlag = input.IsPressed(Keyboard::Keys::F);

        if (spawnFlag)
        {
            TransformC transform = RandomPosTransformC({ 0.0f, 3.0f, 0.0f }, { 5.0f, 1.0f, 5.0f });

            transform.scale = Vector3{ 0.5f, 0.5f, 0.5f };

            TransformC localOffset = { .position = { 0.0f, 1.0f, 0.0f } };

            AddEnemy(ecs, "skelly.glb", transform, localOffset);

            LOG_DEBUG("Spawned entity");
        }

        {
#ifdef USING_IMGUI
            DebugSettings& dbgSettings = ecs.Context<DebugSettings>();

            static std::array<char, 128> modelPath = { "littleguy.glb" };
            static bool                  physics   = false;
            static Vector3               spawnPos  = { 0.0f, 1.0f, 0.0f };
            static Vector3               spawnScl  = { 1.0f, 1.0f, 1.0f };

            if (dbgSettings.useImgui)
            {
                ImGui::SeparatorText("Model Spawner");

                ImGui::InputText("Model path", modelPath.data(), modelPath.size());
                ImGui::DragFloat3("Position", &spawnPos.x, 0.05f);
                ImGui::DragFloat3("Scale", &spawnScl.x, 0.05f, 0.01f, 100.0f);
                ImGui::Checkbox("Physics", &physics);

                if (ImGui::Button("Spawn Model"))
                {
                    TransformC t{};
                    t.position = spawnPos;
                    t.scale    = spawnScl;

                    auto e = AddModelComponent(ecs, modelPath.data(), t);
                    if (physics)
                    {
                        auto& assets    = ecs.ContextRef<AssetManager>();
                        auto& modelC    = ecs.Get<ModelComponent>(e);
                        auto& transform = ecs.Get<TransformC>(e);

                        ecs.Emplace<RigidBody>(
                            e,
                            CreateBoxBody(ecs, assets, modelC.id, transform, Vector3{ 0.5f, 0.5f, 0.5f }));
                    }
                }
            }
#endif
        }

        return false;
    }

    /*****************************************************************************
    * GUI SYSTEMS
    *****************************************************************************/
    bool DebugGuiStartSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (GameRunning(ecs) || GamePaused(ecs))
            return false;

#ifdef USING_IMGUI
        auto& input    = ecs.ContextRef<Input>();
        auto& settings = ecs.Context<DebugSettings>();

        if (input.IsPressed(DirectX::Keyboard::Keys::I))
            settings.useImgui = !settings.useImgui;

        if (settings.useImgui)
        {
            static bool open = settings.useImgui;

            // flags: add some padding to fit the data
            ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
            ImGui::Begin("ImGuiSettings", &settings.useImgui, flags);

            settings.startedImgui = true;
        }
#endif
        return false;
    }

    bool DebugGuiEndSystem::OnUpdate(ECSManager& ecs, float dt)
    {
#ifdef USING_IMGUI

        auto& settings = ecs.Context<DebugSettings>();
        if (settings.startedImgui)
        {
            ImGui::End();
            settings.startedImgui = false;
        }
#endif
        return false;
    }

    bool Game::DebugLevelEditorSystem::OnUpdate(ECSManager& ecs, float dt)
    {
#ifdef USING_IMGUI
        auto& dbgSettings = ecs.Context<DebugSettings>();

        // early out
        if (!dbgSettings.useImgui)
            return false;

        auto playerTransform = ecs.TryGet<TransformC>(ecs.View<PlayerTag>().front());

        if (!playerTransform)
            return false;

        static PointLight       pLight{};
        static SpotLight        sLight{};
        static DirectionalLight dLight{};

        static Entity e = ecs.Create();

        const auto& [x, y, z] = playerTransform->position;
        ImGui::Text("Player Position: %.2f %.2f %.2f", x, y, z);
        ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);

        if (ImGui::TreeNode("Scene Components"))
        {
            // Add new light into the scene
            ImGui::SeparatorText("Lights");

            static bool drawDebugLights = true;
            static bool depthOn         = true;

            ImGui::Checkbox("Draw Debug Lights", &drawDebugLights);
            if (drawDebugLights)
            {
                ImGui::SameLine();
                ImGui::Checkbox("Enable Depth Draw", &depthOn);
            }

            DepthMode dm = depthOn ? DepthMode::DepthRead : DepthMode::Overlay;

            const char* items[] = {
                ToString(LightType::DIRECTIONAL),
                ToString(LightType::POINT),
                ToString(LightType::SPOT),
            };

            static LightType selected     = LightType::DIRECTIONAL;
            const char*      preview_text = items[static_cast<uint32_t>(selected)];

            if (ImGui::BeginCombo("Light", preview_text))
            {
                for (int n = 0; n < std::size(items); n++)
                {
                    const bool is_selected = (static_cast<int>(selected) == n);
                    if (ImGui::Selectable(items[n], is_selected))
                        selected = static_cast<LightType>(n);

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (selected == LightType::DIRECTIONAL)
            {
                DirectionalLight light;
            }
            else if (selected == LightType::POINT)
            {
                ecs.EmplaceOrReplace<PointLight>(e, pLight);
                ecs.Remove<SpotLight>(e);
                ecs.Remove<DirectionalLight>(e);

                ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&pLight.color));
                ImGui::DragFloat3("Position", reinterpret_cast<float*>(&pLight.position), 0.1f);
                ImGui::DragFloat("Intensity", &pLight.intensity, 0.1f, 0.1f, 64.0f, "%.2f");

                ImGui::DragFloat("Min Range", &pLight.minRange, 0.1f, 0.1f, pLight.maxRange, "%.2f");
                ImGui::DragFloat("Max Range", &pLight.maxRange, 0.1f, pLight.minRange, 64.0f, "%.2f");

                constexpr Vector3 ex = { 0.2f, 0.2f, 0.2f };

                if (drawDebugLights)
                {
                    DBG_DRAW.AddAABBCenterExtents(pLight.position, ex, pLight.color, dm);
                    DBG_DRAW.AddSphere(pLight.position, pLight.maxRange, 20, pLight.color, dm);
                }
            }
            else if (selected == LightType::SPOT)
            {
                ecs.EmplaceOrReplace<SpotLight>(e, sLight);
                ecs.Remove<PointLight>(e);
                ecs.Remove<DirectionalLight>(e);

                ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&sLight.color));
                ImGui::DragFloat3("Position", reinterpret_cast<float*>(&sLight.position), 0.1f);
                ImGui::DragFloat3("Direction", reinterpret_cast<float*>(&sLight.direction), 0.1f, 0.0f, 1.0f);
                ImGui::DragFloat("Intensity", &sLight.intensity, 0.1f, 0.1f, 64.0f);
                ImGui::DragFloat("Inner Angle", &sLight.innerAngleRadians, 0.1f, 0.1f, sLight.outerAngleRadians);
                ImGui::DragFloat("Outer Angle", &sLight.outerAngleRadians, 0.1f, sLight.innerAngleRadians, PI_DIV2);
                ImGui::DragFloat("Max Range", &sLight.maxRange, 0.1f, 0.1f, 64.0f, "%.2f");

                if (drawDebugLights)
                {

                    DBG_DRAW_CONE(
                        sLight.position,
                        sLight.direction,
                        sLight.outerAngleRadians,
                        sLight.maxRange,
                        8,
                        sLight.color);
                }
            }

            if (ImGui::Button("Add Light"))
            {
                // we just override the existing entity with a new one
                e = ecs.Create();
            }

            ImGui::TreePop();
        }

#endif
        return false;
    }

    bool DebugDrawZonesSystem::OnUpdate(ECSManager& ecs, float dt)
    {
        if (auto settings = ecs.TryContext<DebugSettings>(); !settings->drawZones)
            return false;

        for (auto [zone, boundary] : ecs.View<ZoneBoundary>().each())
        {
            Vector3 center  = boundary.box.Center;
            Vector3 extents = boundary.box.Extents;

            DBG_DRAW.AddAABBCenterExtents(center, extents, Colors::Red, DepthMode::Overlay);
        }
        return false;
    }
} // namespace Game

#endif // ifdef _DEBUG

#undef LOG_TAG
