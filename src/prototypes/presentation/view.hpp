#pragma once

#include "../entity/appearance/light.hpp"
#include "../entity/appearance/indirect_light_probe.hpp"
#include "../entity/scene/camera.hpp"
#include "../systems/light_ray.hpp"
#include "item.hpp"

#include <vector>

enum class ViewKind : unsigned char
{
    Unknown = 0,
    ShadowCubemapFace,
    Scene3D,
    Overlay2D
};

enum class ViewRenderMode : unsigned char
{
    Shaded = 0,
    Wireframe
};

struct ViewPrototype
{
    bool IsValid() const { return kind != ViewKind::Unknown; }
    ViewKind kind = ViewKind::Unknown;
    ViewRenderMode renderMode = ViewRenderMode::Shaded;
    CameraPrototype camera;
    std::vector<LightPrototype> directLights;
    std::vector<IndirectLightProbePrototype> indirectLightProbes;
    // Transitional archive/experiment lane. Scene BRDF lighting should migrate
    // through LightPrototype instead of extending this path.
    std::vector<LightRayEmitterPrototype> lightRayEmitters;
    std::vector<ItemPrototype> items;
};
