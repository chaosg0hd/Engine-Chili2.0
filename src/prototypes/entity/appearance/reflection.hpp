#pragma once

#include "color.hpp"
#include "../../math/math.hpp"

#include <algorithm>

struct ReflectionPrototype
{
    ColorPrototype reflectionColor = ColorPrototype(1.0f, 1.0f, 1.0f, 1.0f);
    float reflectivity = 0.0f;
    float roughness = 1.0f;

    bool IsReflective() const
    {
        return reflectivity > 0.0f;
    }

    Vector3 ComputeReflectedDirection(const Vector3& incomingDirection, const Vector3& surfaceNormal) const
    {
        const Vector3 incoming = Normalize(incomingDirection);
        const Vector3 normal = Normalize(surfaceNormal);
        if (Length(incoming) <= 0.000001f || Length(normal) <= 0.000001f)
        {
            return incomingDirection;
        }

        return Normalize(incoming - (normal * (2.0f * Dot(incoming, normal))));
    }

    float ApplyReflectivityToIntensity(float incomingIntensity) const
    {
        return incomingIntensity * std::clamp(reflectivity, 0.0f, 1.0f);
    }
};
