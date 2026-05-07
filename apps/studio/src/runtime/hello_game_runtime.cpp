#include "runtime/hello_game_runtime.hpp"

namespace studio_runtime
{
    void HelloGameRuntime::BeginPlay(const ProjectRuntimeDesc& project)
    {
        m_projectName = project.projectName.empty() ? project.projectId : project.projectName;
        m_activeScenePath = project.defaultScenePath.empty() ? "scenes/main.scene" : project.defaultScenePath;
        m_elapsedSeconds = 0.0F;
        m_sceneRuntime.ResetToMinimalScene();
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
            frame.renderFrame = m_sceneRuntime.BuildFrame();
            frame.hasRenderFrame = true;
            return;
        }

        m_elapsedSeconds += deltaSeconds;
        const std::string runtimeText = m_projectName.empty()
            ? "Hello World from Game Runtime"
            : "Hello World from " + m_projectName + " Runtime";
        frame.textOutput = runtimeText + " | Active Scene: " + m_activeScenePath;
        frame.renderFrame = m_sceneRuntime.BuildFrame();
        frame.hasRenderFrame = true;
        frame.exitRequested = input.escapePressed;
    }
}
