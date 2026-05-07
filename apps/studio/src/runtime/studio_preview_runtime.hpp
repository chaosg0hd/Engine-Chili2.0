#pragma once

#include "runtime/runtime_game_api.hpp"
#include "runtime/scene_runtime.hpp"

#include <string>

namespace studio_runtime
{
    class StudioPreviewRuntime final : public IGameRuntime
    {
    public:
        void BeginPlay(const ProjectRuntimeDesc& project) override;
        void EndPlay() override;
        void Tick(float deltaSeconds, const RuntimeInput& input, RuntimeFrame& frame) override;

    private:
        std::string ReadSceneLabel(const ProjectRuntimeDesc& project) const;

        SceneRuntime m_sceneRuntime;
        std::string m_projectName;
        std::string m_activeScenePath;
        std::string m_sceneLabel;
        bool m_playing = false;
    };
}

