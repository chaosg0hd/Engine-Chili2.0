#include "runtime/studio_runtime_host.hpp"

namespace studio_runtime
{
    bool StudioRuntimeHost::Play(const ProjectRuntimeDesc& project, std::string& outError)
    {
        if (project.projectId.empty())
        {
            outError = "No project is open.";
            return false;
        }

        if (project.runtimeName.empty())
        {
            outError = "Opened project does not declare a runtime.";
            return false;
        }

        if (m_state != StudioRuntimePlayState::Stopped)
        {
            Stop();
        }

        m_runtime = m_registry.Create(project.runtimeName);
        if (!m_runtime)
        {
            outError = "Runtime is not registered: " + project.runtimeName + ".";
            return false;
        }

        m_activeProject = project;
        m_runtime->BeginPlay(project);
        m_state = StudioRuntimePlayState::Playing;
        m_viewportText = "Running " + project.runtimeName + " for " + project.projectName + ".";
        m_renderFrame = FramePrototype{};
        m_hasRenderFrame = false;
        return true;
    }

    void StudioRuntimeHost::Stop()
    {
        if (m_runtime && m_state != StudioRuntimePlayState::Stopped)
        {
            m_runtime->EndPlay();
        }

        m_runtime.reset();
        m_state = StudioRuntimePlayState::Stopped;
        m_viewportText = "No runtime running.";
        m_renderFrame = FramePrototype{};
        m_hasRenderFrame = false;
        m_activeProject = ProjectRuntimeDesc{};
    }

    void StudioRuntimeHost::TogglePause()
    {
        if (m_state == StudioRuntimePlayState::Playing)
        {
            m_state = StudioRuntimePlayState::Paused;
            return;
        }

        if (m_state == StudioRuntimePlayState::Paused)
        {
            m_state = StudioRuntimePlayState::Playing;
        }
    }

    void StudioRuntimeHost::Tick(float deltaSeconds, const RuntimeInput& input)
    {
        if (m_state == StudioRuntimePlayState::Stopped)
        {
            m_viewportText = "No runtime running.";
            return;
        }

        if (m_state == StudioRuntimePlayState::Paused)
        {
            m_viewportText = "Runtime paused.";
            return;
        }

        if (!m_runtime)
        {
            Stop();
            return;
        }

        RuntimeFrame frame;
        m_runtime->Tick(deltaSeconds, input, frame);
        m_viewportText = frame.textOutput.empty() ? "No runtime output." : frame.textOutput;
        m_renderFrame = std::move(frame.renderFrame);
        m_hasRenderFrame = frame.hasRenderFrame;

        if (frame.exitRequested)
        {
            Stop();
        }
    }

    StudioRuntimePlayState StudioRuntimeHost::GetState() const
    {
        return m_state;
    }

    std::string StudioRuntimeHost::GetStateName() const
    {
        switch (m_state)
        {
        case StudioRuntimePlayState::Playing:
            return "Playing";
        case StudioRuntimePlayState::Paused:
            return "Paused";
        case StudioRuntimePlayState::Stopped:
        default:
            return "Stopped";
        }
    }

    const std::string& StudioRuntimeHost::GetViewportText() const
    {
        return m_viewportText;
    }

    const FramePrototype& StudioRuntimeHost::GetRenderFrame() const
    {
        return m_renderFrame;
    }

    bool StudioRuntimeHost::HasRenderFrame() const
    {
        return m_hasRenderFrame;
    }

    const ProjectRuntimeDesc& StudioRuntimeHost::GetActiveProject() const
    {
        return m_activeProject;
    }
}
