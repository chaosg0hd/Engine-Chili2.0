#include "grid_render_compiler.hpp"

#include "line_render_compiler.hpp"
#include "../entity/appearance/color.hpp"
#include "../entity/geometry/line.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    float SnapToGrid(float value, float origin, float cellSize)
    {
        return (std::round((value - origin) / cellSize) * cellSize) + origin;
    }
}

void GridRenderCompiler::Append(
    const GridPrototype& grid,
    const CameraPrototype& camera,
    std::vector<RenderItemData>& outItems)
{
    if (grid.extent <= 0.0f || grid.cellSize <= 0.0f)
    {
        return;
    }

    const float cellSize = grid.cellSize;
    const float dashLength = std::max(cellSize * 0.16f, 0.01f);
    const float gapLength = std::max(cellSize * 0.20f, 0.0125f);
    const int majorEvery = std::max(1, grid.majorLineEvery);
    const float centerX = SnapToGrid(camera.pose.position.x, grid.origin.x, cellSize);
    const float centerZ = SnapToGrid(camera.pose.position.z, grid.origin.z, cellSize);
    const int halfLineCount = static_cast<int>(std::floor(grid.extent / cellSize));

    const int centerGridX = static_cast<int>(std::round((centerX - grid.origin.x) / cellSize));
    const int centerGridZ = static_cast<int>(std::round((centerZ - grid.origin.z) / cellSize));
    for (int index = -halfLineCount; index <= halfLineCount; ++index)
    {
        const float offset = static_cast<float>(index) * cellSize;
        const float worldX = centerX + offset;
        const float worldZ = centerZ + offset;
        const int gridIndexX = centerGridX + index;
        const int gridIndexZ = centerGridZ + index;
        const bool majorX = (std::abs(gridIndexX) % majorEvery) == 0;
        const bool majorZ = (std::abs(gridIndexZ) % majorEvery) == 0;
        const ColorPrototype& lineColorX = majorX ? grid.majorLineColor : grid.minorLineColor;
        const ColorPrototype& lineColorZ = majorZ ? grid.majorLineColor : grid.minorLineColor;
        const float thicknessX = majorX ? (grid.lineThickness * 1.5f) : grid.lineThickness;
        const float thicknessZ = majorZ ? (grid.lineThickness * 1.5f) : grid.lineThickness;

        LinePrototype lineAlongZ;
        lineAlongZ.SetSegment(
            Vector3(worldX, grid.origin.y + 0.002f, centerZ - grid.extent),
            Vector3(worldX, grid.origin.y + 0.002f, centerZ + grid.extent));
        LineRenderCompiler::Append(
            lineAlongZ,
            lineColorX,
            thicknessX,
            grid.extent * 2.0f,
            LineRenderStyle::Broken,
            dashLength,
            gapLength,
            outItems);

        LinePrototype lineAlongX;
        lineAlongX.SetSegment(
            Vector3(centerX - grid.extent, grid.origin.y + 0.001f, worldZ),
            Vector3(centerX + grid.extent, grid.origin.y + 0.001f, worldZ));
        LineRenderCompiler::Append(
            lineAlongX,
            lineColorZ,
            thicknessZ,
            grid.extent * 2.0f,
            LineRenderStyle::Broken,
            dashLength,
            gapLength,
            outItems);
    }
}
