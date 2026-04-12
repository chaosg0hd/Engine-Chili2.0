#pragma once

#include "../entity/appearance/color.hpp"
#include "../entity/appearance/material.hpp"
#include "../math/math.hpp"

#include <cstdint>

struct LightRayEmitterPrototype
{
    Vector3 origin = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 direction = Vector3(0.0f, -1.0f, 0.0f);
    ColorPrototype color;
    float intensity = 1.0f;
    std::uint32_t rayCount = 1U;
    std::uint32_t maxBounceCount = 0U;
    std::uint32_t randomSeed = 0U;
    float spreadAngleRadians = 0.0f;
    float maxDistance = 100.0f;
    bool enabled = true;

    void SetDirection(const Vector3& value)
    {
        direction = Normalize(value);
        if (Length(direction) <= 0.000001f)
        {
            direction = Vector3(0.0f, -1.0f, 0.0f);
        }
    }

    bool IsValid() const
    {
        return !enabled ||
            (Length(direction) > 0.000001f &&
             intensity >= 0.0f &&
             rayCount > 0U &&
             spreadAngleRadians >= 0.0f &&
             maxDistance > 0.0f);
    }
};

struct LightRayEmission
{
    std::uint32_t emitterIndex = 0U;
    std::uint32_t rayIndex = 0U;
    std::uint32_t bounceDepth = 0U;
    Vector3 origin = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 direction = Vector3(0.0f, -1.0f, 0.0f);
    ColorPrototype color;
    float intensity = 1.0f;
    float maxDistance = 100.0f;
};

enum class LightRayHitState : unsigned char
{
    Miss = 0,
    Hit
};

struct LightRayTraceResult
{
    LightRayEmission emittedRay;
    LightRayHitState hitState = LightRayHitState::Miss;
    Vector3 hitPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 hitNormal = Vector3(0.0f, 1.0f, 0.0f);
    float hitDistance = 0.0f;
    MaterialPrototype hitMaterial;

    bool DidHit() const
    {
        return hitState == LightRayHitState::Hit;
    }
};

struct LightRayPlaneTraceShape
{
    Vector3 point = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 normal = Vector3(0.0f, 1.0f, 0.0f);
    MaterialPrototype material;
};

struct LightRayBoxTraceShape
{
    Vector3 center = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 halfExtents = Vector3(0.5f, 0.5f, 0.5f);
    MaterialPrototype material;
};

enum class LightRayTraceShapeKind : unsigned char
{
    Plane = 0,
    Box
};

struct LightRayTraceShape
{
    LightRayTraceShapeKind kind = LightRayTraceShapeKind::Plane;
    LightRayPlaneTraceShape plane;
    LightRayBoxTraceShape box;
};
