#include "line_render_compiler.hpp"

#include "../math/math.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kMinimumLineThickness = 0.0005f;
    constexpr float kMinimumLineLength = 0.0005f;
    constexpr float kMinimumBrokenStepLength = 0.0005f;

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

    void AppendSolidLineItem(
        const Vector3& start,
        const Vector3& end,
        const ColorPrototype& color,
        float thickness,
        std::vector<RenderItemData>& outItems)
    {
        RenderItemData itemData;
        itemData.kind = RenderItemDataKind::Line3D;
        itemData.line3D.start = RenderVector3(start.x, start.y, start.z);
        itemData.line3D.end = RenderVector3(end.x, end.y, end.z);
        itemData.line3D.color = color.ToArgb();
        itemData.line3D.thickness = std::max(thickness, kMinimumLineThickness);
        outItems.push_back(itemData);
    }
}

void LineRenderCompiler::Append(
    const LinePrototype& line,
    const ColorPrototype& color,
    float thickness,
    float fallbackLength,
    LineRenderStyle style,
    float brokenDashLength,
    float brokenGapLength,
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

    if (style != LineRenderStyle::Broken)
    {
        AppendSolidLineItem(start, end, color, thickness, outItems);
        return;
    }

    const float dashLength = std::max(brokenDashLength, kMinimumBrokenStepLength);
    const float gapLength = std::max(brokenGapLength, 0.0f);
    const float patternLength = dashLength + gapLength;
    if (patternLength <= kMinimumBrokenStepLength)
    {
        AppendSolidLineItem(start, end, color, thickness, outItems);
        return;
    }

    const Vector3 direction = delta * (1.0f / length);
    float distance = 0.0f;
    while (distance < length)
    {
        const float segmentStartDistance = distance;
        const float segmentEndDistance = std::min(distance + dashLength, length);
        if ((segmentEndDistance - segmentStartDistance) >= kMinimumLineLength)
        {
            AppendSolidLineItem(
                start + (direction * segmentStartDistance),
                start + (direction * segmentEndDistance),
                color,
                thickness,
                outItems);
        }

        distance += patternLength;
    }
}
