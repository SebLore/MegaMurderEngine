#pragma once

#include <cstdint>
#include <vector>

#include "LightComponent.h"

namespace Murder
{
    struct LightCollectionCpu
    {
        std::vector<DirectionalLight> directional;
        std::vector<PointLight>       point;
        std::vector<SpotLight>        spot;

        bool     shadowsEnabled         = true;
        uint32_t maxShadowCastingLights = 8;

        void Clear()
        {
            directional.clear();
            point.clear();
            spot.clear();
        }

        void Reserve(uint32_t dirCount, uint32_t pointCount, uint32_t spotCount)
        {
            directional.reserve(dirCount);
            point.reserve(pointCount);
            spot.reserve(spotCount);
        }

        void Add(const DirectionalLight& l) { directional.push_back(l); }
        void Add(const PointLight& l) { point.push_back(l); }
        void Add(const SpotLight& l) { spot.push_back(l); }

        void Add(DirectionalLight&& l) { directional.emplace_back(std::move(l)); }
        void Add(PointLight&& l) { point.emplace_back(std::move(l)); }
        void Add(SpotLight&& l) { spot.emplace_back(std::move(l)); }

        void Add(const Light& light)
        {
            std::visit([this](const auto& l) { Add(l); }, light);
        }

        [[nodiscard]] uint32_t DirectionalCount() const { return static_cast<uint32_t>(directional.size()); }

        [[nodiscard]] uint32_t PointCount() const { return static_cast<uint32_t>(point.size()); }

        [[nodiscard]] uint32_t SpotCount() const { return static_cast<uint32_t>(spot.size()); }

        [[nodiscard]] uint32_t TotalCount() const { return DirectionalCount() + PointCount() + SpotCount(); }

        [[nodiscard]] uint32_t ShadowCasterCount() const
        {
            uint32_t count = 0;

            if (!shadowsEnabled)
                return 0;

            for (const auto& l : directional)
                if (l.shadow.enabled)
                    ++count;

            for (const auto& l : spot)
                if (l.shadow.enabled)
                    ++count;

            for (const auto& l : point)
                if (l.castsShadow)
                    ++count; // cubemap shadows later

            return count;
        }
    };

} // namespace Murder
