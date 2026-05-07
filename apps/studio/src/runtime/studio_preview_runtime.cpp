#include "runtime/studio_preview_runtime.hpp"

#include "studio/file_proxy.hpp"

#include <cctype>
#include <sstream>

namespace studio_runtime
{
    namespace
    {
        std::string Trim(const std::string& value)
        {
            std::size_t first = 0U;
            while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])) != 0)
            {
                ++first;
            }

            std::size_t last = value.size();
            while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1U])) != 0)
            {
                --last;
            }

            return value.substr(first, last - first);
        }

        std::string ReadKeyValue(const std::string& text, const std::string& key)
        {
            std::stringstream stream(text);
            std::string line;
            while (std::getline(stream, line))
            {
                const std::size_t equals = line.find('=');
                if (equals == std::string::npos)
                {
                    continue;
                }

                if (Trim(line.substr(0U, equals)) == key)
                {
                    return Trim(line.substr(equals + 1U));
                }
            }

            return std::string();
        }
    }

    void StudioPreviewRuntime::BeginPlay(const ProjectRuntimeDesc& project)
    {
        m_projectName = project.projectName.empty() ? project.projectId : project.projectName;
        m_activeScenePath = project.defaultScenePath.empty() ? "scenes/main.scene" : project.defaultScenePath;
        m_sceneLabel = ReadSceneLabel(project);
        m_sceneRuntime.ResetToMinimalScene();
        m_playing = true;
    }

    void StudioPreviewRuntime::EndPlay()
    {
        m_playing = false;
    }

    void StudioPreviewRuntime::Tick(float, const RuntimeInput& input, RuntimeFrame& frame)
    {
        if (input.middleMouseDown)
        {
            const float orbitSpeed = 0.0065f;
            const float yaw = static_cast<float>(input.mouseDeltaX) * orbitSpeed;
            const float pitch = static_cast<float>(-input.mouseDeltaY) * orbitSpeed;
            m_sceneRuntime.OrbitCamera(yaw, pitch);
        }

        frame.renderFrame = m_sceneRuntime.BuildFrame();
        frame.hasRenderFrame = true;
        frame.exitRequested = input.escapePressed;

        if (!m_playing)
        {
            frame.textOutput = "Studio preview stopped.";
            return;
        }

        const std::string sceneName = m_sceneLabel.empty() ? m_activeScenePath : m_sceneLabel;
        frame.textOutput = "Studio Preview | Project: " + m_projectName + " | Scene: " + sceneName;
    }

    std::string StudioPreviewRuntime::ReadSceneLabel(const ProjectRuntimeDesc& project) const
    {
        if (project.projectRootPath.empty())
        {
            return std::string();
        }

        studio::FileProxy files;
        std::string sceneText;
        std::string error;
        const std::string scenePath = project.defaultScenePath.empty()
            ? project.projectRootPath + "/scenes/main.scene"
            : project.projectRootPath + "/" + project.defaultScenePath;
        if (!files.ReadText(scenePath, sceneText, error))
        {
            return std::string();
        }

        const std::string name = ReadKeyValue(sceneText, "name");
        return name;
    }
}
