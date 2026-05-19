#include "engine/script/script_module.hpp"

#include "engine/script/behaviors/rotate_self_script.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace engine::script
{
    namespace
    {
        std::string ReadFileText(const std::string& path)
        {
            std::ifstream input(path);
            if (!input.is_open())
            {
                return std::string();
            }

            return std::string(
                (std::istreambuf_iterator<char>(input)),
                std::istreambuf_iterator<char>());
        }

        std::string ExtractJsonStringField(const std::string& text, const std::string& fieldName)
        {
            const std::string key = "\"" + fieldName + "\"";
            const std::size_t keyPos = text.find(key);
            if (keyPos == std::string::npos)
            {
                return std::string();
            }

            const std::size_t colonPos = text.find(':', keyPos + key.size());
            if (colonPos == std::string::npos)
            {
                return std::string();
            }

            const std::size_t firstQuote = text.find('"', colonPos + 1U);
            if (firstQuote == std::string::npos)
            {
                return std::string();
            }

            const std::size_t secondQuote = text.find('"', firstQuote + 1U);
            if (secondQuote == std::string::npos || secondQuote <= firstQuote)
            {
                return std::string();
            }

            return text.substr(firstQuote + 1U, secondQuote - firstQuote - 1U);
        }
    }

    ScriptModule::ScriptModule()
    {
        RegisterBuiltIns();
    }

    ScriptRegistry& ScriptModule::Registry()
    {
        return m_registry;
    }

    const ScriptRegistry& ScriptModule::Registry() const
    {
        return m_registry;
    }

    void ScriptModule::LoadFromProxyFolder(const std::string& proxyFolderPath)
    {
        m_registry = ScriptRegistry{};
        RegisterBuiltIns();

        if (proxyFolderPath.empty())
        {
            return;
        }

        std::error_code error;
        const std::filesystem::path scriptsPath = std::filesystem::path(proxyFolderPath) / "scripts";
        if (!std::filesystem::exists(scriptsPath, error))
        {
            return;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(scriptsPath, error))
        {
            if (error || !entry.is_regular_file())
            {
                continue;
            }

            const std::string text = ReadFileText(entry.path().string());
            const std::string id = ExtractJsonStringField(text, "id");
            const std::string method = ExtractJsonStringField(text, "method");
            if (id.empty() || method != "this.rotate")
            {
                continue;
            }

            ScriptPrototype rotateScript;
            rotateScript
                .SetName(id)
                .SetFactory(&CreateRotateSelfScript);
            m_registry.Register(rotateScript);
        }
    }

    void ScriptModule::RegisterBuiltIns()
    {
    }
}
