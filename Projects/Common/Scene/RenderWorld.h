#pragma once

#include "Sprite.h"
#include "SpriteText.h" // Engine2D
#include "ModelInstance.h"
#include "SceneComponents.h"

#include <cstdint>
#include <vector>

namespace Murder
{
    // single struct to hold RenderWorld
    struct RenderWorld
    {
        // camera
        Camera camera{};

        // lights
        std::vector<PointLight>       pointLights;
        std::vector<DirectionalLight> directionalLights;
        std::vector<SpotLight>        spotLights;
        Vector3                       ambientLight = { 0.1f, 0.1f, 0.1f };

        // renderables
        std::vector<uint32_t>      visibleModelsIndices;
        std::vector<ModelInstance> models;
        std::vector<Sprite*>       sprites;
        std::vector<SpriteText*>   spriteTexts;

        // helper function to clear
        void Clear()
        {
            models.clear();
            visibleModelsIndices.clear();
            directionalLights.clear();
            pointLights.clear();
            spotLights.clear();

            sprites.clear();
            spriteTexts.clear();
        }
    };

} // namespace Murder
