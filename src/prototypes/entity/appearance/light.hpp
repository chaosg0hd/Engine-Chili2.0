#pragma once

#include "../../systems/light_ray.hpp"
#include "color.hpp"
#include "light_visibility.hpp"

// Light prototypes are the future-facing public authoring contract for illumination.
// They describe the phenomenon in conceptual lanes so translation code can adapt them
// into a renderer/backend execution path without flattening intent too early.

enum class LightEmitterKind : unsigned char
{
    Point = 0,
    Directional,
    Spot
};

struct LightEmitterPrototype
{
    LightEmitterKind kind = LightEmitterKind::Point;
    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 direction = Vector3(0.0f, -1.0f, 0.0f);
    ColorPrototype color;
    float intensity = 0.0f;
    float range = 1.0f;
    float innerConeDegrees = 0.0f;
    float outerConeDegrees = 0.0f;
};

enum class LightTransportKind : unsigned char
{
    DirectAnalytic = 0,
    RasterAssisted,
    Traced,
    Baked,
    Hybrid
};

struct LightTransportPrototype
{
    LightTransportKind kind = LightTransportKind::DirectAnalytic;
};

enum class LightVisibilityKind : unsigned char
{
    None = 0,
    RasterShadowMap,
    RasterShadowCubemap,
    Traced
};

struct LightVisibilityPrototype
{
    LightVisibilityKind kind = LightVisibilityKind::None;
    LightVisibilityPolicyPrototype policy;
};

enum class LightInteractionKind : unsigned char
{
    DirectBrdf = 0
};

struct LightInteractionPrototype
{
    LightInteractionKind kind = LightInteractionKind::DirectBrdf;
    bool directBrdf = true;
    bool reflection = false;
    bool transmission = false;
};

enum class LightRealizationKind : unsigned char
{
    RealtimeDynamic = 0,
    LowCostDynamic,
    BudgetLimitedDynamic,
    Baked
};

struct LightRealizationPrototype
{
    LightRealizationKind kind = LightRealizationKind::RealtimeDynamic;
};

struct LightPrototype
{
    LightEmitterPrototype emitter;
    LightTransportPrototype transport;
    LightVisibilityPrototype visibility;
    LightInteractionPrototype interaction;
    LightRealizationPrototype realization;
    bool enabled = true;
};

// Transitional compatibility contract for the legacy point-light authoring lane.
// New code should author against LightPrototype and use this only when adapting
// older scene/sandbox call sites during migration.
enum class SceneLightKind : unsigned char
{
    Point = 0
};

struct SceneLightPrototype
{
    SceneLightKind kind = SceneLightKind::Point;
    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
    ColorPrototype color;
    float intensity = 0.0f;
    float range = 1.0f;
    LightVisibilityPolicyPrototype visibilityPolicy;
    bool enabled = true;

    LightPrototype ToLightPrototype() const
    {
        LightPrototype light;
        light.emitter.kind = LightEmitterKind::Point;
        light.emitter.position = position;
        light.emitter.color = color;
        light.emitter.intensity = intensity;
        light.emitter.range = range;
        light.transport.kind = LightTransportKind::DirectAnalytic;
        light.visibility.kind = visibilityPolicy.IsRasterCubemapEnabled()
            ? LightVisibilityKind::RasterShadowCubemap
            : LightVisibilityKind::None;
        light.visibility.policy = visibilityPolicy;
        light.interaction.kind = LightInteractionKind::DirectBrdf;
        light.interaction.directBrdf = true;
        light.realization.kind = LightRealizationKind::RealtimeDynamic;
        light.enabled = enabled;
        return light;
    }

    static SceneLightPrototype FromLightPrototype(const LightPrototype& light)
    {
        SceneLightPrototype legacy;
        legacy.kind = SceneLightKind::Point;
        legacy.position = light.emitter.position;
        legacy.color = light.emitter.color;
        legacy.intensity = light.emitter.intensity;
        legacy.range = light.emitter.range;
        legacy.visibilityPolicy = light.visibility.policy;
        legacy.enabled = light.enabled;
        return legacy;
    }
};

// Transitional light-ray authoring contract for experimentation and archived paths.
// It remains separate from BRDF-oriented scene lights until a later integration pass.
struct LightRayPrototype
{
    LightRayEmitterPrototype emitter;

    void SetRay(const Vector3& origin, const Vector3& direction)
    {
        emitter.origin = origin;
        emitter.SetDirection(direction);
    }

    bool IsValid() const
    {
        return emitter.IsValid();
    }
};
