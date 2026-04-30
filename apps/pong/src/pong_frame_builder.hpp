#pragma once

#include "pong_game.hpp"

#include "prototypes/entity/appearance/color.hpp"
#include "prototypes/presentation/frame.hpp"

struct PongFrameStyle
{
    ColorPrototype lineColor = ColorPrototype::FromBytes(142, 168, 162, 102);
    ColorPrototype leftPaddleColor = ColorPrototype::FromBytes(86, 214, 163);
    ColorPrototype rightPaddleColor = ColorPrototype::FromBytes(255, 200, 87);
    ColorPrototype ballColor = ColorPrototype::FromBytes(244, 247, 245);
    ColorPrototype serveColor = ColorPrototype::FromBytes(125, 226, 209);
};

class PongFrameBuilder
{
public:
    static FramePrototype Build(const PongGame& game);
    static FramePrototype Build(const PongGame& game, const PongFrameStyle& style);
};
