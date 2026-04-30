#include "runtime/pong_runtime.hpp"

#include "studio/file_proxy.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace studio_runtime
{
    namespace
    {
        float Axis(bool positive, bool negative)
        {
            float axis = 0.0F;
            if (positive)
            {
                axis += 1.0F;
            }
            if (negative)
            {
                axis -= 1.0F;
            }

            return std::clamp(axis, -1.0F, 1.0F);
        }

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

        bool ParseHexColor(const std::string& value, std::uint32_t& outColor)
        {
            std::string text = Trim(value);
            if (text.empty())
            {
                return false;
            }

            if (text.size() > 0U && text[0U] == '#')
            {
                text.erase(text.begin());
            }
            else if (text.size() > 2U && text[0U] == '0' && (text[1U] == 'x' || text[1U] == 'X'))
            {
                text.erase(0U, 2U);
            }

            if (text.size() == 6U)
            {
                text = "FF" + text;
            }

            if (text.size() != 8U)
            {
                return false;
            }

            std::uint32_t color = 0U;
            for (const char ch : text)
            {
                color <<= 4U;
                if (ch >= '0' && ch <= '9')
                {
                    color |= static_cast<std::uint32_t>(ch - '0');
                }
                else if (ch >= 'a' && ch <= 'f')
                {
                    color |= static_cast<std::uint32_t>(10 + ch - 'a');
                }
                else if (ch >= 'A' && ch <= 'F')
                {
                    color |= static_cast<std::uint32_t>(10 + ch - 'A');
                }
                else
                {
                    return false;
                }
            }

            outColor = color;
            return true;
        }

        void ApplyColor(const std::string& text, const std::string& key, std::uint32_t& color)
        {
            std::uint32_t parsed = 0U;
            if (ParseHexColor(ReadKeyValue(text, key), parsed))
            {
                color = parsed;
            }
        }
    }

    void PongRuntime::BeginPlay(const ProjectRuntimeDesc& project)
    {
        PongGameSystem::Reset(m_game);
        LoadHotEditableProjectData(project);
        m_playing = true;
    }

    void PongRuntime::EndPlay()
    {
        m_playing = false;
        PongGameSystem::Reset(m_game);
    }

    void PongRuntime::Tick(float deltaSeconds, const RuntimeInput& input, RuntimeFrame& frame)
    {
        if (!m_playing)
        {
            frame.textOutput = "Pong runtime is stopped.";
            frame.renderFrame = PongFrameBuilder::Build(m_game, m_frameStyle);
            frame.hasRenderFrame = true;
            return;
        }

        PongInput pongInput;
        pongInput.leftPaddleAxis = Axis(input.leftUpDown, input.leftDownDown);
        pongInput.rightPaddleAxis = Axis(input.rightUpDown, input.rightDownDown);
        pongInput.leftPaddleAiEnabled = false;
        pongInput.rightPaddleAiEnabled = true;
        pongInput.resetRequested = input.resetPressed;
        pongInput.serveRequested = input.servePressed || m_game.ball.waitingForServe;

        PongGameSystem::Step(m_game, pongInput, deltaSeconds);

        frame.textOutput = m_runtimeMessage;
        frame.renderFrame = PongFrameBuilder::Build(m_game, m_frameStyle);
        frame.hasRenderFrame = true;
        frame.exitRequested = input.escapePressed;
    }

    void PongRuntime::LoadHotEditableProjectData(const ProjectRuntimeDesc& project)
    {
        m_frameStyle = PongFrameStyle{};
        m_runtimeMessage = project.projectName.empty() ? "PongRuntime" : project.projectName + " Runtime";

        studio::FileProxy files;
        std::string error;
        std::string configText;
        if (files.ReadText(project.projectRootPath + "/config/game.config", configText, error))
        {
            const std::string message = ReadKeyValue(configText, "runtime_message");
            if (!message.empty())
            {
                m_runtimeMessage = message;
            }

            ApplyColor(configText, "line_color", m_frameStyle.lineColor);
            ApplyColor(configText, "left_paddle_color", m_frameStyle.leftPaddleColor);
            ApplyColor(configText, "right_paddle_color", m_frameStyle.rightPaddleColor);
            ApplyColor(configText, "ball_color", m_frameStyle.ballColor);
            ApplyColor(configText, "serve_color", m_frameStyle.serveColor);
        }

        std::string sceneText;
        const std::string scenePath = project.defaultScenePath.empty()
            ? project.projectRootPath + "/scenes/main.scene"
            : project.projectRootPath + "/" + project.defaultScenePath;
        if (files.ReadText(scenePath, sceneText, error))
        {
            const std::string message = ReadKeyValue(sceneText, "viewport_message");
            if (!message.empty())
            {
                m_runtimeMessage = message;
            }

            ApplyColor(sceneText, "line_color", m_frameStyle.lineColor);
            ApplyColor(sceneText, "left_paddle_color", m_frameStyle.leftPaddleColor);
            ApplyColor(sceneText, "right_paddle_color", m_frameStyle.rightPaddleColor);
            ApplyColor(sceneText, "ball_color", m_frameStyle.ballColor);
            ApplyColor(sceneText, "serve_color", m_frameStyle.serveColor);
        }
    }
}
