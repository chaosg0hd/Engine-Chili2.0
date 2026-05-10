#include "line_render_compiler.hpp"

#include "object_render_compiler.hpp"

#include "../entity/appearance/color.hpp"
#include "../entity/object/mesh.hpp"
#include "../entity/object/object.hpp"
#include "../math/math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float kMinimumLineThickness = 0.0005f;
    constexpr float kMinimumLineLength = 0.0005f;

    float ResolveRenderedLength(const LinePrototype& line, float fallbackLength)
    {
        if (line.IsSegment())
        {
            return line.GetLength();
        }

        return std::max(fallbackLength, kMinimumLineLength);
    }

    Vector3 ResolveRenderedStart(const LinePrototype& line, float renderedLength)
    {
        if (line.IsSegment())
        {
            return line.GetStart().position;
        }

        if (line.IsRay())
        {
            return line.origin;
        }

        return line.origin - (line.GetDirection() * (renderedLength * 0.5f));
    }

    Vector3 ResolveRenderedEnd(const LinePrototype& line, float renderedLength)
    {
        if (line.IsSegment())
        {
            return line.GetEnd().position;
        }

        if (line.IsRay())
        {
            return line.origin + (line.GetDirection() * renderedLength);
        }

        return line.origin + (line.GetDirection() * (renderedLength * 0.5f));
    }
}

void LineRenderCompiler::Append(
    const LinePrototype& line,
    const ColorPrototype& color,
    float thickness,
    float fallbackLength,
    std::vector<RenderItemData>& outItems)
{
    if (!line.IsValid())
    {
        return;
    }

    const float renderedLength = ResolveRenderedLength(line, fallbackLength);
    if (!std::isfinite(renderedLength) || renderedLength < kMinimumLineLength)
    {
        return;
    }

    const Vector3 start = ResolveRenderedStart(line, renderedLength);
    const Vector3 end = ResolveRenderedEnd(line, renderedLength);
    const Vector3 delta = end - start;
    const float length = Length(delta);
    if (length < kMinimumLineLength)
    {
        return;
    }

    const Vector3 direction = delta * (1.0f / length);
    const float yawRadians = std::atan2(direction.x, direction.z);
    const float horizontalLength = std::sqrt((direction.x * direction.x) + (direction.z * direction.z));
    const float pitchRadians = -std::atan2(direction.y, horizontalLength);
    const float lineThickness = std::max(thickness, kMinimumLineThickness);

    ObjectPrototype lineObject;
    lineObject.transform.translation = start + (delta * 0.5f);
    lineObject.transform.rotationRadians = Vector3(pitchRadians, yawRadians, 0.0f);
    lineObject.transform.scale = Vector3(lineThickness, lineThickness, length);

    MeshPrototype& mesh = lineObject.GetPrimaryMesh();
    mesh.builtInKind = BuiltInMeshKind::Cube;
    mesh.material.baseLayer.albedo = color;
    mesh.material.brdf.diffuseStrength = 0.0f;
    mesh.material.brdf.ambientStrength = 0.0f;
    mesh.material.emissive.enabled = true;
    mesh.material.emissive.color = color;
    mesh.material.emissive.intensity = 1.0f;
    mesh.shadowParticipation.casts = false;
    mesh.shadowParticipation.receives = false;

    ObjectRenderCompiler::Append(lineObject, outItems);
}
