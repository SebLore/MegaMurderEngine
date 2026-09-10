#pragma once
#include "ModelAsset.h"

namespace Murder::Assets
{
    /**
     * @brief Build the local-to-model transformation matrices for each node in the model.
     * @param model The model asset containing the nodes.
     * @param outNodeWorld The output vector to store the local-to-model transformation matrices of the nodes.
     */
    inline void BuildNodeLocalToModel(const ModelAsset& model, std::vector<Matrix>& outNodeWorld)
    {
        const size_t nodeCount = model.nodes.size();

        outNodeWorld.clear();
        outNodeWorld.resize(nodeCount, Matrix::Identity);

        for (size_t i = 0; i < nodeCount; ++i)
        {
            const auto& node = model.nodes[i];

            Matrix localToModel = node.local;

            if (node.parent >= 0 && static_cast<size_t>(node.parent) < nodeCount)
                localToModel = localToModel * outNodeWorld[static_cast<size_t>(node.parent)];

            outNodeWorld[i] = localToModel;
        }
    }

    /**
     * @brief Build the world transformation matrices for each node in the model.
     * @param model The model asset containing the nodes.
     * @param instanceWorld The world transformation matrix of the model instance.
     * @param outNodeWorld The output vector to store the world transformation matrices of the nodes.
     */
    inline void BuildNodeWorld(const ModelAsset& model, const Matrix& instanceWorld, std::vector<Matrix>& outNodeWorld)
    {
        BuildNodeLocalToModel(model, outNodeWorld);

        for (Matrix& m : outNodeWorld)
            m = m * instanceWorld;
    }

} // namespace Murder::Assets
