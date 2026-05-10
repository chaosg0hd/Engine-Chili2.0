#include "snap_types.hpp"

namespace
{
    SnapCandidate BuildCandidate(
        SnapCandidateType type,
        const Vector3& worldPosition,
        std::uint64_t objectId,
        const char* debugLabel)
    {
        SnapCandidate candidate;
        candidate.type = type;
        candidate.worldPosition = worldPosition;
        if (objectId != 0)
        {
            candidate.hasObjectId = true;
            candidate.objectId = objectId;
        }
        if (debugLabel)
        {
            candidate.debugLabel = debugLabel;
        }
        return candidate;
    }
}

SnapCandidate MakeVertexCandidate(
    const Vector3& worldPosition,
    std::uint64_t objectId,
    const char* debugLabel)
{
    return BuildCandidate(SnapCandidateType::Point, worldPosition, objectId, debugLabel);
}

SnapCandidate MakePointCandidate(
    const Vector3& worldPosition,
    std::uint64_t objectId,
    const char* debugLabel)
{
    return BuildCandidate(SnapCandidateType::Point, worldPosition, objectId, debugLabel);
}

SnapCandidate MakeObjectOriginCandidate(
    const Vector3& worldPosition,
    std::uint64_t objectId,
    const char* debugLabel)
{
    return BuildCandidate(SnapCandidateType::ObjectOrigin, worldPosition, objectId, debugLabel);
}

