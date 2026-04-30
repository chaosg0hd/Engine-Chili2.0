#pragma once

#include "runtime/runtime_game_api.hpp"

#include "pong_frame_builder.hpp"
#include "pong_game.hpp"

namespace studio_runtime
{
    class PongRuntime final : public IGameRuntime
    {
    public:
        void BeginPlay(const ProjectRuntimeDesc& project) override;
        void EndPlay() override;
        void Tick(float deltaSeconds, const RuntimeInput& input, RuntimeFrame& frame) override;

    private:
        void LoadHotEditableProjectData(const ProjectRuntimeDesc& project);

        PongGame m_game;
        PongFrameStyle m_frameStyle;
        std::string m_runtimeMessage = "PongRuntime";
        bool m_playing = false;
    };
}
