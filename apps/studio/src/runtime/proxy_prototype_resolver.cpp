#include "runtime/proxy_prototype_resolver.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

namespace studio_runtime
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

        std::size_t FindMatchingClose(const std::string& text, std::size_t openPos, char openCh, char closeCh)
        {
            int depth = 0;
            for (std::size_t i = openPos; i < text.size(); ++i)
            {
                if (text[i] == openCh)
                {
                    ++depth;
                }
                else if (text[i] == closeCh)
                {
                    --depth;
                    if (depth == 0)
                    {
                        return i;
                    }
                }
            }
            return std::string::npos;
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

        std::string ExtractObjectSlice(const std::string& text, const std::string& fieldName)
        {
            const std::string key = "\"" + fieldName + "\"";
            const std::size_t keyPos = text.find(key);
            if (keyPos == std::string::npos)
            {
                return std::string();
            }
            const std::size_t open = text.find('{', keyPos + key.size());
            if (open == std::string::npos)
            {
                return std::string();
            }
            const std::size_t close = FindMatchingClose(text, open, '{', '}');
            if (close == std::string::npos || close <= open)
            {
                return std::string();
            }
            return text.substr(open, close - open + 1U);
        }

        std::vector<std::string> ExtractStringArrayField(const std::string& text, const std::string& fieldName)
        {
            std::vector<std::string> values;
            const std::string key = "\"" + fieldName + "\"";
            const std::size_t keyPos = text.find(key);
            if (keyPos == std::string::npos)
            {
                return values;
            }
            const std::size_t open = text.find('[', keyPos + key.size());
            if (open == std::string::npos)
            {
                return values;
            }
            const std::size_t close = FindMatchingClose(text, open, '[', ']');
            if (close == std::string::npos || close <= open)
            {
                return values;
            }

            std::size_t cursor = open + 1U;
            while (cursor < close)
            {
                const std::size_t firstQuote = text.find('"', cursor);
                if (firstQuote == std::string::npos || firstQuote >= close)
                {
                    break;
                }
                const std::size_t secondQuote = text.find('"', firstQuote + 1U);
                if (secondQuote == std::string::npos || secondQuote > close)
                {
                    break;
                }
                values.push_back(text.substr(firstQuote + 1U, secondQuote - firstQuote - 1U));
                cursor = secondQuote + 1U;
            }
            return values;
        }

        bool ExtractJsonBoolField(const std::string& text, const std::string& fieldName, bool defaultValue)
        {
            const std::string key = "\"" + fieldName + "\"";
            const std::size_t keyPos = text.find(key);
            if (keyPos == std::string::npos)
            {
                return defaultValue;
            }

            const std::size_t colonPos = text.find(':', keyPos + key.size());
            if (colonPos == std::string::npos)
            {
                return defaultValue;
            }

            const std::size_t truePos = text.find("true", colonPos + 1U);
            const std::size_t falsePos = text.find("false", colonPos + 1U);
            if (truePos != std::string::npos && (falsePos == std::string::npos || truePos < falsePos))
            {
                return true;
            }
            if (falsePos != std::string::npos && (truePos == std::string::npos || falsePos < truePos))
            {
                return false;
            }
            return defaultValue;
        }

        bool ExtractVector3Field(const std::string& text, const std::string& fieldName, Vector3& out)
        {
            const std::string key = "\"" + fieldName + "\"";
            const std::size_t keyPos = text.find(key);
            if (keyPos == std::string::npos)
            {
                return false;
            }
            const std::size_t open = text.find('[', keyPos + key.size());
            const std::size_t close = text.find(']', open);
            if (open == std::string::npos || close == std::string::npos || close <= open)
            {
                return false;
            }

            std::string values = text.substr(open + 1U, close - open - 1U);
            std::replace(values.begin(), values.end(), ',', ' ');
            std::stringstream stream(values);
            if (!(stream >> out.x >> out.y >> out.z))
            {
                return false;
            }
            return true;
        }
    }

    bool ProxyPrototypeResolver::LoadFromProxyFolder(const std::string& proxyFolderPath)
    {
        Clear();
        if (proxyFolderPath.empty())
        {
            return false;
        }

        std::error_code error;
        const std::filesystem::path prototypesPath = std::filesystem::path(proxyFolderPath) / "prototypes";
        if (!std::filesystem::exists(prototypesPath, error))
        {
            return false;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(prototypesPath, error))
        {
            if (error || !entry.is_regular_file())
            {
                continue;
            }

            const std::string text = ReadFileText(entry.path().string());
            ResolvedPrototype prototype;
            prototype.id = ExtractJsonStringField(text, "id");
            if (prototype.id.empty())
            {
                continue;
            }

            prototype.type = ExtractJsonStringField(text, "type");
            const std::string assets = ExtractObjectSlice(text, "assets");
            const std::string defaults = ExtractObjectSlice(text, "defaults");
            const std::string defaultTransform = ExtractObjectSlice(defaults, "transform");
            prototype.meshAsset = ExtractJsonStringField(assets, "mesh");
            prototype.materialAsset = ExtractJsonStringField(assets, "material");
            prototype.lightAsset = ExtractJsonStringField(assets, "light");
            prototype.visible = ExtractJsonBoolField(defaults, "visible", true);
            const bool hasPosition = ExtractVector3Field(defaultTransform, "position", prototype.defaultTransform.translation);
            const bool hasRotation = ExtractVector3Field(defaultTransform, "rotation", prototype.defaultTransform.rotationRadians);
            const bool hasScale = ExtractVector3Field(defaultTransform, "scale", prototype.defaultTransform.scale);
            prototype.hasDefaultTransform = hasPosition || hasRotation || hasScale;
            if (!prototype.lightAsset.empty())
            {
                prototype.objectKind = "Light";
            }

            m_prototypes[prototype.id] = prototype;
        }

        return !m_prototypes.empty();
    }

    void ProxyPrototypeResolver::Clear()
    {
        m_prototypes.clear();
    }

    bool ProxyPrototypeResolver::TryResolve(const std::string& prototypeId, ResolvedPrototype& outPrototype) const
    {
        const auto found = m_prototypes.find(prototypeId);
        if (found == m_prototypes.end())
        {
            return false;
        }

        outPrototype = found->second;
        return true;
    }
}
