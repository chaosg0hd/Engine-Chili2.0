#pragma once

#include "math.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

struct CubeSampleVertexPrototype
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct CubeFacePrototype
{
    std::array<int, 4> vertices;
    Vector3 normal;
    std::uint32_t color = 0xFFFFFFFFU;
};

inline Vector3 RotateCubeSamplePoint(const Vector3& point, double time)
{
    const float yaw = static_cast<float>(time * 0.82);
    const float pitch = static_cast<float>(time * 0.54);
    const float roll = static_cast<float>(time * 0.27);

    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);
    const float cr = std::cos(roll);
    const float sr = std::sin(roll);

    Vector3 rotated;
    rotated.x = (point.x * cy) + (point.z * sy);
    rotated.y = point.y;
    rotated.z = (-point.x * sy) + (point.z * cy);

    rotated = Vector3(
        rotated.x,
        (rotated.y * cp) - (rotated.z * sp),
        (rotated.y * sp) + (rotated.z * cp));

    return Vector3(
        (rotated.x * cr) - (rotated.y * sr),
        (rotated.x * sr) + (rotated.y * cr),
        rotated.z);
}

inline CubeSampleVertexPrototype ProjectCubeSamplePoint(
    const Vector3& point,
    float centerX,
    float centerY,
    float depthOffset)
{
    const float worldZ = point.z + depthOffset;
    const float cameraDistance = 3.2f;
    const float projectionScale = 0.82f;
    const float denominator = std::max(0.50f, cameraDistance - worldZ);

    return CubeSampleVertexPrototype{
        centerX + ((point.x * projectionScale) / denominator),
        centerY + ((point.y * projectionScale) / denominator),
        worldZ
    };
}

inline float ComputeCubeEdgeSign(
    float px,
    float py,
    const CubeSampleVertexPrototype& a,
    const CubeSampleVertexPrototype& b)
{
    return ((px - b.x) * (a.y - b.y)) - ((a.x - b.x) * (py - b.y));
}

inline bool IsPointInCubeSampleTriangle(
    float px,
    float py,
    const CubeSampleVertexPrototype& a,
    const CubeSampleVertexPrototype& b,
    const CubeSampleVertexPrototype& c)
{
    const float d1 = ComputeCubeEdgeSign(px, py, a, b);
    const float d2 = ComputeCubeEdgeSign(px, py, b, c);
    const float d3 = ComputeCubeEdgeSign(px, py, c, a);
    const bool hasNegative = d1 < 0.0f || d2 < 0.0f || d3 < 0.0f;
    const bool hasPositive = d1 > 0.0f || d2 > 0.0f || d3 > 0.0f;
    return !(hasNegative && hasPositive);
}

inline bool IsPointInCubeSampleFace(
    float px,
    float py,
    const std::array<CubeSampleVertexPrototype, 8>& vertices,
    const CubeFacePrototype& face)
{
    return
        IsPointInCubeSampleTriangle(px, py, vertices[face.vertices[0]], vertices[face.vertices[1]], vertices[face.vertices[2]]) ||
        IsPointInCubeSampleTriangle(px, py, vertices[face.vertices[0]], vertices[face.vertices[2]], vertices[face.vertices[3]]);
}

inline float ComputeDistanceToCubeSampleSegment(
    float px,
    float py,
    const CubeSampleVertexPrototype& a,
    const CubeSampleVertexPrototype& b)
{
    const float x = b.x - a.x;
    const float y = b.y - a.y;
    const float lengthSquared = (x * x) + (y * y);
    if (lengthSquared <= 0.000001f)
    {
        const float dx = px - a.x;
        const float dy = py - a.y;
        return std::sqrt((dx * dx) + (dy * dy));
    }

    const float t = std::max(0.0f, std::min(1.0f, (((px - a.x) * x) + ((py - a.y) * y)) / lengthSquared));
    const float closestX = a.x + (x * t);
    const float closestY = a.y + (y * t);
    const float dx = px - closestX;
    const float dy = py - closestY;
    return std::sqrt((dx * dx) + (dy * dy));
}

inline float ConsumeSampleWork(float seed, unsigned int iterations)
{
    float value = seed;
    for (unsigned int index = 0U; index < iterations; ++index)
    {
        const float wave = std::sin(value * 1.713f) + std::cos((value + static_cast<float>(index)) * 0.371f);
        value = std::sqrt(std::fabs((value * 0.911f) + wave) + 0.001f);
    }

    return value;
}
