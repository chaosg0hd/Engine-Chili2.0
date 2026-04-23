#include "infinite_plane_render_compiler.hpp"

#include "line_render_compiler.hpp"
#include "object_render_compiler.hpp"

#include "../entity/appearance/material.hpp"
#include "../entity/geometry/line.hpp"
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
    basePlane.transform.translation = Vector3(camera.pose.position.x, plane.origin.y, camera.pose.position.z);
    basePlane.transform.scale = Vector3(plane.extent * 2.0f, 1.0f, plane.extent * 2.0f);
    basePlane.GetPrimaryMesh().material.baseLayer.albedo = ColorPrototype::FromArgb(plane.baseColor);
    ObjectRenderCompiler::Append(basePlane, outItems);

    const float clampedMinorSpacing = plane.minorGridSpacing;
    const float clampedMajorSpacing =
        (plane.majorGridSpacing >= clampedMinorSpacing) ? plane.majorGridSpacing : clampedMinorSpacing;
    const int lineCount = static_cast<int>(std::floor((plane.extent * 2.0f) / clampedMinorSpacing));
    const int halfLineCount = lineCount / 2;
    const float centerX = std::round(camera.pose.position.x / clampedMinorSpacing) * clampedMinorSpacing;
    const float centerZ = std::round(camera.pose.position.z / clampedMinorSpacing) * clampedMinorSpacing;

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

        LinePrototype lineAlongZ;
        lineAlongZ.SetSegment(
            Vector3(worldX, plane.origin.y + 0.002f, centerZ - plane.extent),
            Vector3(worldX, plane.origin.y + 0.002f, centerZ + plane.extent));
        LineRenderCompiler::Append(lineAlongZ, lineColor, thickness, plane.extent * 2.0f, outItems);

        LinePrototype lineAlongX;
        lineAlongX.SetSegment(
            Vector3(centerX - plane.extent, plane.origin.y + 0.001f, worldZ),
            Vector3(centerX + plane.extent, plane.origin.y + 0.001f, worldZ));
        LineRenderCompiler::Append(lineAlongX, lineColor, thickness, plane.extent * 2.0f, outItems);
    }
}
