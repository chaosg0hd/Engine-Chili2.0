#include "studio/studio_template_system.hpp"

#include "studio/file_proxy.hpp"
#include "runtime/default_scene_json.hpp"

#include <cctype>
#include <sstream>

namespace studio
{
    namespace
    {
        std::string JoinVirtualPath(const std::string& left, const std::string& right)
        {
            if (left.empty())
            {
                return right;
            }

            if (right.empty())
            {
                return left;
            }

            return left + "/" + right;
        }

        std::string ToPascalIdentifier(const std::string& projectId)
        {
            std::string identifier;
            bool capitalizeNext = true;

            for (const char character : projectId)
            {
                if (character == '_')
                {
                    capitalizeNext = true;
                    continue;
                }

                const unsigned char value = static_cast<unsigned char>(character);
                if (std::isalnum(value) == 0)
                {
                    capitalizeNext = true;
                    continue;
                }

                if (capitalizeNext)
                {
                    identifier += static_cast<char>(std::toupper(value));
                    capitalizeNext = false;
                }
                else
                {
                    identifier += static_cast<char>(std::tolower(value));
                }
            }

            if (identifier.empty() || std::isdigit(static_cast<unsigned char>(identifier.front())) != 0)
            {
                identifier.insert(identifier.begin(), 'G');
            }

            return identifier;
        }

        std::string BuildArcadeHeader(const std::string& className)
        {
            std::ostringstream output;
            output
                << "#pragma once\n"
                << "\n"
                << "struct AppCapabilities;\n"
                << "\n"
                << "class " << className << "Game\n"
                << "{\n"
                << "public:\n"
                << "    void Reset();\n"
                << "    void Update(AppCapabilities& capabilities);\n"
                << "\n"
                << "private:\n"
                << "    float m_playerX = 0.0f;\n"
                << "    int m_score = 0;\n"
                << "};\n";
            return output.str();
        }

        std::string BuildArcadeMainSource(const std::string& projectId, const std::string& className)
        {
            std::ostringstream output;
            output
                << "#include \"" << projectId << ".hpp\"\n"
                << "\n"
                << "#include \"app/app_capabilities.hpp\"\n"
                << "#include \"core/engine_core.hpp\"\n"
                << "\n"
                << "#include <cstdint>\n"
                << "\n"
                << "int main()\n"
                << "{\n"
                << "    EngineCore core;\n"
                << "    if (!core.Initialize())\n"
                << "    {\n"
                << "        return 1;\n"
                << "    }\n"
                << "\n"
                << "    " << className << "Game game;\n"
                << "    game.Reset();\n"
                << "\n"
                << "    core.SetFrameCallback(\n"
                << "        [&game](AppCapabilities& capabilities)\n"
                << "        {\n"
                << "            game.Update(capabilities);\n"
                << "            if (capabilities.rendering)\n"
                << "            {\n"
                << "                capabilities.rendering->ClearFrame(0xFF101820u);\n"
                << "            }\n"
                << "        });\n"
                << "\n"
                << "    const bool success = core.Run();\n"
                << "    core.Shutdown();\n"
                << "    return success ? 0 : 1;\n"
                << "}\n";
            return output.str();
        }

        std::string BuildArcadeSource(const std::string& projectName, const std::string& projectId, const std::string& className)
        {
            std::ostringstream output;
            output
                << "#include \"" << projectId << ".hpp\"\n"
                << "\n"
                << "#include \"app/app_capabilities.hpp\"\n"
                << "\n"
                << "#include <algorithm>\n"
                << "\n"
                << "void " << className << "Game::Reset()\n"
                << "{\n"
                << "    m_playerX = 0.0f;\n"
                << "    m_score = 0;\n"
                << "}\n"
                << "\n"
                << "void " << className << "Game::Update(AppCapabilities& capabilities)\n"
                << "{\n"
                << "    const float deltaSeconds = capabilities.time\n"
                << "        ? static_cast<float>(std::max(0.0, capabilities.time->GetDeltaSeconds()))\n"
                << "        : 0.0f;\n"
                << "\n"
                << "    float moveAxis = 0.0f;\n"
                << "    if (capabilities.input)\n"
                << "    {\n"
                << "        if (capabilities.input->IsKeyDown(AppKey::Right))\n"
                << "        {\n"
                << "            moveAxis += 1.0f;\n"
                << "        }\n"
                << "        if (capabilities.input->IsKeyDown(AppKey::Left))\n"
                << "        {\n"
                << "            moveAxis -= 1.0f;\n"
                << "        }\n"
                << "        if (capabilities.input->WasKeyPressed(AppKey::Space))\n"
                << "        {\n"
                << "            ++m_score;\n"
                << "        }\n"
                << "        if (capabilities.input->WasKeyPressed(AppKey::R))\n"
                << "        {\n"
                << "            Reset();\n"
                << "        }\n"
                << "    }\n"
                << "\n"
                << "    m_playerX = std::clamp(m_playerX + (moveAxis * deltaSeconds), -1.0f, 1.0f);\n"
                << "\n"
                << "    if (capabilities.logging && m_score == 1)\n"
                << "    {\n"
                << "        capabilities.logging->Info(\"" << projectName << ": Arcade2D template is running.\");\n"
                << "    }\n"
                << "}\n";
            return output.str();
        }

        std::string BuildProjectCMake(const std::string& projectId)
        {
            std::ostringstream output;
            output
                << "cmake_minimum_required(VERSION 3.20)\n"
                << "\n"
                << "project(" << projectId << " LANGUAGES CXX)\n"
                << "\n"
                << "set(CMAKE_CXX_STANDARD 17)\n"
                << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
                << "set(CMAKE_CXX_EXTENSIONS OFF)\n"
                << "\n"
                << "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)\n"
                << "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)\n"
                << "set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)\n"
                << "\n"
                << "get_filename_component(ENGINE_CHILI_ROOT \"${CMAKE_CURRENT_LIST_DIR}/../../..\" ABSOLUTE)\n"
                << "include(\"${ENGINE_CHILI_ROOT}/cmake/compiler_warnings.cmake\")\n"
                << "include(\"${ENGINE_CHILI_ROOT}/cmake/sanitizers.cmake\")\n"
                << "\n"
                << "add_subdirectory(\"${ENGINE_CHILI_ROOT}/core\" \"${CMAKE_BINARY_DIR}/engine_core\")\n"
                << "\n"
                << "add_executable(" << projectId << "\n"
                << "    src/main.cpp\n"
                << "    src/" << projectId << ".cpp\n"
                << "    src/" << projectId << ".hpp\n"
                << ")\n"
                << "\n"
                << "target_include_directories(" << projectId << "\n"
                << "    PRIVATE\n"
                << "        ${CMAKE_CURRENT_SOURCE_DIR}/src\n"
                << "        ${ENGINE_CHILI_ROOT}/src\n"
                << ")\n"
                << "\n"
                << "target_link_libraries(" << projectId << "\n"
                << "    PRIVATE\n"
                << "        engine_core\n"
                << ")\n"
                << "\n"
                << "apply_project_warnings(" << projectId << ")\n"
                << "enable_project_sanitizers(" << projectId << ")\n";
            return output.str();
        }

        std::string BuildMainScene(const std::string&)
        {
            // Default scene template is reusable editor/runtime composition only.
            // It must stay free of gameplay behavior.
            return studio_runtime::BuildDefaultSceneTemplateSceneJson();
        }

        std::string BuildGameConfig()
        {
            return "target_fps = 60\nstartup_scene = scenes/main.scene\n";
        }
    }

    StudioTemplateSystem::StudioTemplateSystem(const FileProxy& files)
        : m_files(files)
    {
    }

    ApplyTemplateResult StudioTemplateSystem::ApplyTemplate(
        const std::string& templateName,
        const std::string& targetProjectPath,
        const std::string& projectName,
        const std::string& projectId) const
    {
        if (templateName == "Arcade2D")
        {
            return ApplyArcade2DTemplate(targetProjectPath, projectName, projectId);
        }

        ApplyTemplateResult result;
        result.error = "Unknown project template: " + templateName;
        return result;
    }

    ApplyTemplateResult StudioTemplateSystem::ApplyArcade2DTemplate(
        const std::string& targetProjectPath,
        const std::string& projectName,
        const std::string& projectId) const
    {
        ApplyTemplateResult result;
        const std::string className = ToPascalIdentifier(projectId);
        std::string error;

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "CMakeLists.txt"),
                BuildProjectCMake(projectId),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "src/main.cpp"),
                BuildArcadeMainSource(projectId, className),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "src/" + projectId + ".hpp"),
                BuildArcadeHeader(className),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "src/" + projectId + ".cpp"),
                BuildArcadeSource(projectName, projectId, className),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "scenes/main.scene"),
                BuildMainScene(projectName),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "config/game.config"),
                BuildGameConfig(),
                error))
        {
            result.error = error;
            return result;
        }

        result.success = true;
        result.message = "Applied Arcade2D template.";
        return result;
    }
}
