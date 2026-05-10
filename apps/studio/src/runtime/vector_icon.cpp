#include "runtime/vector_icon.hpp"

#include <algorithm>
#include <cmath>

namespace studio_runtime
{
    namespace
    {
        constexpr float kPi = 3.14159265358979323846f;

        VectorIconPoint TransformPoint(
            const VectorIcon& icon,
            const VectorIconStyle& style,
            float anchorScreenX,
            float anchorScreenY,
            const VectorIconPoint& point)
        {
            const float localX = (point.x - icon.hotspot.x) * style.scalePixels;
            const float localY = (point.y - icon.hotspot.y) * style.scalePixels;
            return VectorIconPoint{ anchorScreenX + localX, anchorScreenY + localY };
        }
    }

    void VectorIconRenderer::RenderIconToOverlayItems(
        const VectorIcon& icon,
        float anchorScreenX,
        float anchorScreenY,
        const ViewportRect& viewport,
        const VectorIconStyle& style,
        std::vector<ItemPrototype>& outItems) const
    {
        for (const VectorIconCommand& command : icon.commands)
        {
            switch (command.type)
            {
            case VectorIconCommandType::Line:
            {
                const VectorIconPoint a = TransformPoint(icon, style, anchorScreenX, anchorScreenY, command.from);
                const VectorIconPoint b = TransformPoint(icon, style, anchorScreenX, anchorScreenY, command.to);
                AddLinePatch(a.x, a.y, b.x, b.y, style.strokePixels, style.tintArgb, viewport, outItems);
                break;
            }
            case VectorIconCommandType::Circle:
            {
                const VectorIconPoint center = TransformPoint(icon, style, anchorScreenX, anchorScreenY, command.center);
                const float radiusPx = std::max(0.5f, command.radius * style.scalePixels);
                const int segments = 24;
                for (int index = 0; index < segments; ++index)
                {
                    const float t0 = (static_cast<float>(index) / static_cast<float>(segments)) * (2.0f * kPi);
                    const float t1 = (static_cast<float>(index + 1) / static_cast<float>(segments)) * (2.0f * kPi);
                    const float ax = center.x + (std::cos(t0) * radiusPx);
                    const float ay = center.y + (std::sin(t0) * radiusPx);
                    const float bx = center.x + (std::cos(t1) * radiusPx);
                    const float by = center.y + (std::sin(t1) * radiusPx);
                    AddLinePatch(ax, ay, bx, by, style.strokePixels, style.tintArgb, viewport, outItems);
                }
                break;
            }
            case VectorIconCommandType::Path:
            {
                if (command.pathPoints.size() < 2)
                {
                    break;
                }

                for (std::size_t index = 0; index + 1 < command.pathPoints.size(); ++index)
                {
                    const VectorIconPoint a = TransformPoint(icon, style, anchorScreenX, anchorScreenY, command.pathPoints[index]);
                    const VectorIconPoint b = TransformPoint(icon, style, anchorScreenX, anchorScreenY, command.pathPoints[index + 1]);
                    AddLinePatch(a.x, a.y, b.x, b.y, style.strokePixels, style.tintArgb, viewport, outItems);
                }

                if (command.closePath)
                {
                    const VectorIconPoint a = TransformPoint(icon, style, anchorScreenX, anchorScreenY, command.pathPoints.back());
                    const VectorIconPoint b = TransformPoint(icon, style, anchorScreenX, anchorScreenY, command.pathPoints.front());
                    AddLinePatch(a.x, a.y, b.x, b.y, style.strokePixels, style.tintArgb, viewport, outItems);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    void VectorIconRenderer::AddLinePatch(
        float ax,
        float ay,
        float bx,
        float by,
        float strokePixels,
        std::uint32_t color,
        const ViewportRect& viewport,
        std::vector<ItemPrototype>& outItems)
    {
        const float width = static_cast<float>(std::max(1, viewport.width));
        const float height = static_cast<float>(std::max(1, viewport.height));

        const float axNdc = (((ax - static_cast<float>(viewport.x)) / width) * 2.0f) - 1.0f;
        const float ayNdc = 1.0f - (((ay - static_cast<float>(viewport.y)) / height) * 2.0f);
        const float bxNdc = (((bx - static_cast<float>(viewport.x)) / width) * 2.0f) - 1.0f;
        const float byNdc = 1.0f - (((by - static_cast<float>(viewport.y)) / height) * 2.0f);

        const float dx = bxNdc - axNdc;
        const float dy = byNdc - ayNdc;
        const float length = std::sqrt((dx * dx) + (dy * dy));
        if (length <= 0.000001f)
        {
            return;
        }

        const float minDim = std::max(1.0f, std::min(width, height));
        const float strokeNdc = (2.0f * std::max(0.5f, strokePixels)) / minDim;

        ItemPrototype patch;
        patch.kind = ItemKind::ScreenPatch;
        patch.screenPatch.centerX = (axNdc + bxNdc) * 0.5f;
        patch.screenPatch.centerY = (ayNdc + byNdc) * 0.5f;
        patch.screenPatch.halfWidth = strokeNdc * 0.5f;
        patch.screenPatch.halfHeight = length * 0.5f;
        patch.screenPatch.rotationRadians = -std::atan2(dx, dy);
        patch.screenPatch.color = color;
        outItems.push_back(patch);
    }
}

