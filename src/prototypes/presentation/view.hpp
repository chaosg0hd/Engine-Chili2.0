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

struct ViewPrototype
{
    ViewKind kind = ViewKind::Unknown;
    CameraPrototype camera;
    std::vector<LightPrototype> directLights;
    std::vector<IndirectLightProbePrototype> indirectLightProbes;
    // Transitional archive/experiment lane. Scene BRDF lighting should migrate
    // through LightPrototype instead of extending this path.
    std::vector<LightRayEmitterPrototype> lightRayEmitters;
    std::vector<ItemPrototype> items;
};
