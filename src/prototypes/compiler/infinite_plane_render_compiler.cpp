#include "infinite_plane_render_compiler.hpp"

#include "object_render_compiler.hpp"

#include "../entity/appearance/material.hpp"
#include "../entity/object/mesh.hpp"
#include "../entity/object/object.hpp"
#include "../math/math.hpp"

#include <cmath>

void InfinitePlaneRenderCompiler::Append(
    const InfinitePlanePrototype& plane,
    const CameraPrototype& camera,
    std::vector<RenderItemData>& outItems)
{
    if (plane.extent <= 0.0f || plane.minorGridSpacing <= 0.0f)
    {
        return;
    }

    ObjectPrototype basePlane;
    basePlane.GetPrimaryMesh().builtInKind = BuiltInMeshKind::Quad;
    basePlane.transform.translation = Vector3(camera.position.x, plane.origin.y, camera.position.z);
    basePlane.transform.scale = Vector3(plane.extent * 2.0f, 1.0f, plane.extent * 2.0f);
    basePlane.GetPrimaryMesh().material.baseColor = ColorPrototype::FromArgb(plane.baseColor);
    ObjectRenderCompiler::Append(basePlane, outItems);

    const float clampedMinorSpacing = plane.minorGridSpacing;
    const float clampedMajorSpacing =
        (plane.majorGridSpacing >= clampedMinorSpacing) ? plane.majorGridSpacing : clampedMinorSpacing;
    const int lineCount = static_cast<int>(std::floor((plane.extent * 2.0f) / clampedMinorSpacing));
    const int halfLineCount = lineCount / 2;
    const float centerX = std::round(camera.position.x / clampedMinorSpacing) * clampedMinorSpacing;
    const float centerZ = std::round(camera.position.z / clampedMinorSpacing) * clampedMinorSpacing;

    for (int index = -halfLineCount; index <= halfLineCount; ++index)
    {
        const float offset = static_cast<float>(index) * clampedMinorSpacing;
        const float worldX = centerX + offset;
        const float worldZ = centerZ + offset;
        const float majorRemainder = std::fmod(std::fabs(offset), clampedMajorSpacing);
        const bool isMajorLine =
            (majorRemainder < 0.001f) ||
            (std::fabs(majorRemainder - clampedMajorSpacing) < 0.001f);
        const std::uint32_t lineColor = isMajorLine ? plane.majorLineColor : plane.minorLineColor;
        const float thickness = isMajorLine ? (plane.lineThickness * 1.5f) : plane.lineThickness;

        ObjectPrototype lineAlongZ;
        lineAlongZ.GetPrimaryMesh().builtInKind = BuiltInMeshKind::Quad;
        lineAlongZ.transform.translation = Vector3(worldX, plane.origin.y + 0.002f, centerZ);
        lineAlongZ.transform.scale = Vector3(thickness, 1.0f, plane.extent * 2.0f);
        lineAlongZ.GetPrimaryMesh().material.baseColor = ColorPrototype::FromArgb(lineColor);
        ObjectRenderCompiler::Append(lineAlongZ, outItems);

        ObjectPrototype lineAlongX;
        lineAlongX.GetPrimaryMesh().builtInKind = BuiltInMeshKind::Quad;
        lineAlongX.transform.translation = Vector3(centerX, plane.origin.y + 0.001f, worldZ);
        lineAlongX.transform.scale = Vector3(plane.extent * 2.0f, 1.0f, thickness);
        lineAlongX.GetPrimaryMesh().material.baseColor = ColorPrototype::FromArgb(lineColor);
        ObjectRenderCompiler::Append(lineAlongX, outItems);
    }
}
