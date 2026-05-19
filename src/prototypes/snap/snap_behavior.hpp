#pragma once

#include "../math/math.hpp"
#include "snap_types.hpp"

#include <cstdint>
#include <vector>

struct SnapBehaviorPrototype
{
    bool snapCentroid = true;
    bool snapVertices = true;
    bool snapBasePoint = false;
    Vector3 basePoint = Vector3(0.0f, 0.0f, 0.0f);

    void CollectSnapCandidates(
        std::vector<SnapCandidate>& outCandidates,
        const TransformPrototype& worldTransform,
        std::uint64_t objectId = 0) const
    {
        if (snapCentroid)
        {
            outCandidates.push_back(MakeObjectOriginCandidate(
                worldTransform.translation, objectId, "ObjectOrigin"));
        }

        if (snapVertices)
        {
            const float hx = 0.5f;
            const float hy = 0.5f;
            const float hz = 0.5f;
            const Matrix4 worldMatrix = worldTransform.ToMatrix();
            for (int sx = -1; sx <= 1; sx += 2)
            for (int sy = -1; sy <= 1; sy += 2)
            for (int sz = -1; sz <= 1; sz += 2)
            {
                outCandidates.push_back(MakeVertexCandidate(
                    TransformPoint(worldMatrix, Vector3(sx * hx, sy * hy, sz * hz)),
                    objectId, "BBoxCorner"));
            }
        }

        if (snapBasePoint)
        {
            outCandidates.push_back(MakePointCandidate(
                TransformPoint(worldTransform.ToMatrix(), basePoint), objectId, "BasePoint"));
        }
    }
};
