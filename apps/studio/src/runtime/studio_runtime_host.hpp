#pragma once

#include "runtime/runtime_game_api.hpp"
#include "runtime/runtime_registry.hpp"

#include <memory>
#include <string>

namespace studio_runtime
{
    enum class StudioRuntimePlayState
    {
        Stopped,
        Playing,
        Paused
    };

    class StudioRuntimeHost
    {
    public:
        bool Play(const ProjectRuntimeDesc& project, std::string& outError);
        void Stop();
        void TogglePause();
        void Tick(float deltaSeconds, const RuntimeInput& input);

        StudioRuntimePlayState GetState() const;
        std::string GetStateName() const;
        const std::string& GetViewportText() const;
        const FramePrototype& GetRenderFrame() const;
        bool HasRenderFrame() const;
        const ProjectRuntimeDesc& GetActiveProject() const;

    private:
        RuntimeRegistry m_registry;
        std::unique_ptr<IGameRuntime> m_runtime;
        ProjectRuntimeDesc m_activeProject;
        FramePrototype m_renderFrame;
        StudioRuntimePlayState m_state = StudioRuntimePlayState::Stopped;
        std::string m_viewportText = "No runtime running.";
        bool m_hasRenderFrame = false;
    };
}
