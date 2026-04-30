#pragma once

#include "../entity/appearance/color.hpp"
#include "frame.hpp"
#include "item.hpp"
#include "pass.hpp"
#include "view.hpp"

#include <cstddef>
#include <utility>

class OverlayFrameBuilder
{
public:
    explicit OverlayFrameBuilder(std::size_t expectedItems = 0U)
    {
        m_view.kind = ViewKind::Overlay2D;
        m_view.items.reserve(expectedItems);
    }

    OverlayFrameBuilder& Rect(
        float centerX,
        float centerY,
        float halfWidth,
        float halfHeight,
        ColorPrototype color,
        float rotationRadians = 0.0f)
    {
        m_view.items.push_back(MakePatch(
            ItemKind::ScreenPatch,
            centerX,
            centerY,
            halfWidth,
            halfHeight,
            color,
            rotationRadians));
        return *this;
    }

    OverlayFrameBuilder& Hex(
        float centerX,
        float centerY,
        float halfWidth,
        float halfHeight,
        ColorPrototype color,
        float rotationRadians = 0.0f)
    {
        m_view.items.push_back(MakePatch(
            ItemKind::ScreenHexPatch,
            centerX,
            centerY,
            halfWidth,
            halfHeight,
            color,
            rotationRadians));
        return *this;
    }

    FramePrototype Build()
    {
        PassPrototype pass;
        pass.kind = PassKind::Overlay;
        pass.views.push_back(std::move(m_view));

        FramePrototype frame;
        frame.passes.push_back(std::move(pass));
        return frame;
    }

private:
    static ItemPrototype MakePatch(
        ItemKind kind,
        float centerX,
        float centerY,
        float halfWidth,
        float halfHeight,
        ColorPrototype color,
        float rotationRadians)
    {
        ItemPrototype item;
        item.kind = kind;
        item.screenPatch.centerX = centerX;
        item.screenPatch.centerY = centerY;
        item.screenPatch.halfWidth = halfWidth;
        item.screenPatch.halfHeight = halfHeight;
        item.screenPatch.rotationRadians = rotationRadians;
        item.screenPatch.color = color.ToArgb();
        return item;
    }

    ViewPrototype m_view;
};
