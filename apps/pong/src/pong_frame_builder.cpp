#include "pong_frame_builder.hpp"

#include "prototypes/presentation/overlay_frame_builder.hpp"

#include <cmath>

FramePrototype PongFrameBuilder::Build(const PongGame& game)
{
    return Build(game, PongFrameStyle{});
}

FramePrototype PongFrameBuilder::Build(const PongGame& game, const PongFrameStyle& style)
{
    OverlayFrameBuilder frame(64U);

    const PongRules& rules = game.rules;
    frame.Rect(0.0f, rules.arenaHalfHeight, rules.arenaHalfWidth, 0.006f, style.lineColor);
    frame.Rect(0.0f, -rules.arenaHalfHeight, rules.arenaHalfWidth, 0.006f, style.lineColor);

    for (int index = 0; index < 11; ++index)
    {
        const float y = -0.88f + (static_cast<float>(index) * 0.176f);
        frame.Rect(0.0f, y, 0.007f, 0.050f, style.lineColor);
    }

    const float leftX = -rules.arenaHalfWidth + rules.paddleInset;
    const float rightX = rules.arenaHalfWidth - rules.paddleInset;
    frame.Rect(
        leftX,
        game.leftPaddle.centerY,
        rules.paddleHalfWidth,
        rules.paddleHalfHeight,
        style.leftPaddleColor);
    frame.Rect(
        rightX,
        game.rightPaddle.centerY,
        rules.paddleHalfWidth,
        rules.paddleHalfHeight,
        style.rightPaddleColor);

    const ColorPrototype ballColor = game.ball.waitingForServe ? style.serveColor : style.ballColor;
    const float pulse = game.ball.waitingForServe
        ? 1.0f + (0.18f * std::sin(static_cast<float>(game.rallyCount) * 0.37f))
        : 1.0f;
    frame.Hex(
        game.ball.centerX,
        game.ball.centerY,
        rules.ballHalfSize * pulse,
        rules.ballHalfSize * pulse,
        ballColor,
        0.52359877f);

    return frame.Build();
}
