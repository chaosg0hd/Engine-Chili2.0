#include "runtime/hello_game_runtime.hpp"

namespace studio_runtime
{
    void HelloGameRuntime::BeginPlay(const ProjectRuntimeDesc& project)
    {
        m_projectName = project.projectName.empty() ? project.projectId : project.projectName;
        m_elapsedSeconds = 0.0F;
        m_playing = true;
    }

    void HelloGameRuntime::EndPlay()
    {
        m_playing = false;
        m_elapsedSeconds = 0.0F;
    }

    void HelloGameRuntime::Tick(float deltaSeconds, const RuntimeInput& input, RuntimeFrame& frame)
    {
        if (!m_playing)
        {
            frame.textOutput = "No runtime running.";
            return;
        }

        m_elapsedSeconds += deltaSeconds;
        frame.textOutput = m_projectName.empty()
            ? "Hello World from Game Runtime"
            : "Hello World from " + m_projectName + " Runtime";
        frame.exitRequested = input.escapePressed;
    }
}
