#pragma once

#include "runtime/runtime_game_api.hpp"
#include "runtime/scene_runtime.hpp"

namespace studio_runtime
{
    class HelloGameRuntime final : public IGameRuntime
    {
    public:
        void BeginPlay(const ProjectRuntimeDesc& project) override;
        void EndPlay() override;
        void Tick(float deltaSeconds, const RuntimeInput& input, RuntimeFrame& frame) override;

    private:
        std::string m_projectName;
        std::string m_activeScenePath;
        SceneRuntime m_sceneRuntime;
        float m_elapsedSeconds = 0.0F;
        bool m_playing = false;
    };
}
