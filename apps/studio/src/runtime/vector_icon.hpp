#pragma once

#include "prototypes/presentation/item.hpp"
#include "modules/render/render_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace studio_runtime
{
    struct VectorIconPoint
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    enum class VectorIconCommandType : std::uint8_t
    {
        Line = 0,
        Circle,
        Path
    };

    struct VectorIconCommand
    {
        VectorIconCommandType type = VectorIconCommandType::Line;
        VectorIconPoint from{};
        VectorIconPoint to{};
        VectorIconPoint center{};
        float radius = 0.0f;
        std::vector<VectorIconPoint> pathPoints;
        bool closePath = false;
    };

    struct VectorIcon
    {
        std::string id;
        float viewBoxMinX = 0.0f;
        float viewBoxMinY = 0.0f;
        float viewBoxWidth = 24.0f;
        float viewBoxHeight = 24.0f;
        VectorIconPoint hotspot{ 0.0f, 0.0f };
        std::vector<VectorIconCommand> commands;
    };

    struct VectorIconStyle
    {
        float scalePixels = 1.0f;
        float strokePixels = 1.5f;
        std::uint32_t tintArgb = 0xCCFFFFFFu;
    };

    class VectorIconRenderer
    {
    public:
        void RenderIconToOverlayItems(
            const VectorIcon& icon,
            float anchorScreenX,
            float anchorScreenY,
            const ViewportRect& viewport,
            const VectorIconStyle& style,
            std::vector<ItemPrototype>& outItems) const;

    private:
        static void AddLinePatch(
            float ax,
            float ay,
            float bx,
            float by,
            float strokePixels,
            std::uint32_t color,
            const ViewportRect& viewport,
            std::vector<ItemPrototype>& outItems);
    };
}

