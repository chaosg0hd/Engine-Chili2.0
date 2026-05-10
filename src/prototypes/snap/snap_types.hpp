#pragma once

#include "../math/math.hpp"

#include <cstdint>
#include <string>

enum class SnapCandidateType : std::uint8_t
{
    HelperPoint = 0,
    Point,
    Edge,
    FacePlane,
    Grid,
    ObjectOrigin,
    BoundingBoxCorner,
    BoundingBoxCenter
};

enum class SnapToolKind : std::uint8_t
{
    Generic = 0,
    Translate,
    Rotate,
    Scale
};

struct SnapRay
{
    Vector3 origin = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 direction = Vector3(0.0f, 0.0f, 1.0f);
};

struct SnapProbe
{
    bool hasPoint = false;
    Vector3 point = Vector3(0.0f, 0.0f, 0.0f);
    bool hasRay = false;
    SnapRay ray{};
};

struct SnapContext
{
    SnapProbe probe{};
    SnapToolKind activeTool = SnapToolKind::Generic;
};

struct SnapCandidate
{
    SnapCandidateType type = SnapCandidateType::Point;

    // Fallback world position for point-like candidates.
    Vector3 worldPosition = Vector3(0.0f, 0.0f, 0.0f);

    // Optional feature data for projected candidates.
    bool hasEdge = false;
    Vector3 edgeStart = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 edgeEnd = Vector3(0.0f, 0.0f, 0.0f);

    bool hasPlane = false;
    Vector3 planePoint = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 planeNormal = Vector3(0.0f, 1.0f, 0.0f);

    int priorityBias = 0;
    std::uint64_t objectId = 0;
    bool hasObjectId = false;
    bool blockedHint = false;
    bool hasScreenDistance = false;
    float screenDistancePx = 0.0f;
    std::string debugLabel;
};

SnapCandidate MakeVertexCandidate(
    const Vector3& worldPosition,
    std::uint64_t objectId = 0,
    const char* debugLabel = "Vertex");

SnapCandidate MakePointCandidate(
    const Vector3& worldPosition,
    std::uint64_t objectId = 0,
    const char* debugLabel = "Point");

SnapCandidate MakeObjectOriginCandidate(
    const Vector3& worldPosition,
    std::uint64_t objectId = 0,
    const char* debugLabel = "ObjectOrigin");

struct SnapResult
{
    bool didSnap = false;
    Vector3 snapPosition = Vector3(0.0f, 0.0f, 0.0f);
    SnapCandidateType snapType = SnapCandidateType::Point;
    std::uint64_t sourceObjectId = 0;
    bool hasSourceObjectId = false;
    std::string reason;
    float confidence = 0.0f;
};
