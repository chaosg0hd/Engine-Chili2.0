#include "engine/game_runtime_api.hpp"

#include "pong_app.hpp"

namespace
{
    int RunPongPreview()
    {
        PongApp app;
        return app.Run() ? 0 : 1;
    }
}

extern "C" GAME_RUNTIME_API_EXPORT const GameRuntimeApi* get_game_api()
{
    static const GameRuntimeApi api{
        1U,
        "PongRuntime",
        &RunPongPreview
    };

    return &api;
}
