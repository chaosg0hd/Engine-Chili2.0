#pragma once

#include "absorption.hpp"
#include "color.hpp"
#include "reflection.hpp"

#include <cstdint>
#include <string>

using MaterialHandle = std::uint32_t;

struct MaterialLayerPrototype
{
    ColorPrototype albedo = ColorPrototype(1.0f, 1.0f, 1.0f, 1.0f);
    std::string albedoAssetId;
    std::string normalAssetId;
    std::string heightAssetId;
    float albedoBlend = 0.0f;
    float roughness = 0.5f;
    float normalStrength = 1.0f;
    float heightStrength = 0.0f;
};

// Transitional direct-BRDF controls for the current real-time realization.
// Ambient is retained separately for now and is not part of the direct-light law.
struct MaterialBrdfPrototype
{
    float ambientStrength = 0.0f;
    float diffuseStrength = 1.0f;
    float specularStrength = 0.0f;
    float specularPower = 1.0f;
};

struct MaterialEmissivePrototype
{
    bool enabled = false;
    ColorPrototype color = ColorPrototype(1.0f, 1.0f, 1.0f, 1.0f);
    float intensity = 0.0f;
};

struct MaterialPrototype : public ReflectionPrototype, public AbsorptionPrototype
{
    MaterialHandle handle = 0;
    MaterialLayerPrototype baseLayer;
    MaterialLayerPrototype detailLayer;
    MaterialBrdfPrototype brdf;
    MaterialEmissivePrototype emissive;

    ColorPrototype ResolveSurfaceAlbedo() const
    {
        return baseLayer.albedo;
    }
};
