#pragma once
#include "CameraGpu.h"
#include "MaterialGpu.h"
#include "TransformGpu.h"
#include "Core/AssetTags.h"
#include "LightCollection.h"

#include <vector>
#include <array>

#include "RenderSprite.h"

namespace Murder::Render
{
    struct DrawList
    {
        // Per model part
        struct Item
        {
            MeshId meshId{};

            TransformGpuData transform;
            MaterialGpuData  materialData;

            TextureId textureId{};
            // TODO: use
            std::array<TextureId, TextureSlotCount> textureIds{};

            uint32_t submeshIndex = 0;

            bool inView      = true;
            bool transparent = false;
            bool viewModel   = false;
            bool dynamic     = false;

            uint32_t materialIndex = 0;
        };

        struct Shadow
        {
            enum class Kind : uint32_t
            {
                Directional = 0,
                Spot        = 1
                // TODO: point
            };

            Kind          kind        = Kind::Directional;
            uint32_t      lightIndex  = 0; // index into directionalLights or spotLights
            uint32_t      shadowSlice = 0; // slice in Texture2DArray shadowMaps
            CameraGpuData camera{};        // view/proj/vp for the shadow pass

            float depthBias  = 0.0f;
            float normalBias = 0.0f;
            float strength   = 1.0f;
            float _pad0      = 0.0f;
        };

        std::vector<Item>         items;
        std::vector<RenderSprite> sprites;
        std::vector<SpriteText*>  textSprites;
        std::vector<Shadow>       shadows;

        CameraGpuData camera;

        std::vector<DirectionalLightGpuData> dirLights;
        std::vector<PointLightGpuData>       pointLights;
        std::vector<SpotLightGpuData>        spotLights;

        // for the cbuffer, so we know how many of each light type we have, and the ambient color
        LightCollectionBufferData lightCollection;

        void Clear()
        {
            items.clear();
            dirLights.clear();
            pointLights.clear();
            spotLights.clear();
            sprites.clear();
            textSprites.clear();
            shadows.clear();

            camera          = {};
            lightCollection = {};
        }
    };
} // namespace Murder::Render
