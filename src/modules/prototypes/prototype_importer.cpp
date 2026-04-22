#include "prototype_importer.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{
    PrototypeId MakePrototypeId(const std::string& value)
    {
        std::uint32_t hash = 2166136261U;
        for (unsigned char character : value)
        {
            hash ^= character;
            hash *= 16777619U;
        }

        return hash != 0U ? hash : 1U;
    }

    bool TryFindKeyValueStart(
        const std::string& text,
        const std::string& key,
        std::size_t& outValueStart)
    {
        const std::string quotedKey = "\"" + key + "\"";
        const std::size_t keyStart = text.find(quotedKey);
        if (keyStart == std::string::npos)
        {
            return false;
        }

        const std::size_t separator = text.find(':', keyStart + quotedKey.size());
        if (separator == std::string::npos)
        {
            return false;
        }

        outValueStart = separator + 1U;
        while (outValueStart < text.size() && std::isspace(static_cast<unsigned char>(text[outValueStart])) != 0)
        {
            ++outValueStart;
        }

        return outValueStart < text.size();
    }

    bool ExtractStringField(const std::string& text, const std::string& key, std::string& outValue)
    {
        std::size_t valueStart = 0U;
        if (!TryFindKeyValueStart(text, key, valueStart) || text[valueStart] != '"')
        {
            outValue.clear();
            return false;
        }

        const std::size_t valueEnd = text.find('"', valueStart + 1U);
        if (valueEnd == std::string::npos)
        {
            outValue.clear();
            return false;
        }

        outValue = text.substr(valueStart + 1U, valueEnd - valueStart - 1U);
        return true;
    }

    bool ExtractIntField(const std::string& text, const std::string& key, int& outValue)
    {
        std::size_t valueStart = 0U;
        if (!TryFindKeyValueStart(text, key, valueStart))
        {
            return false;
        }

        char* parseEnd = nullptr;
        const long value = std::strtol(text.c_str() + valueStart, &parseEnd, 10);
        if (parseEnd == text.c_str() + valueStart)
        {
            return false;
        }

        outValue = static_cast<int>(value);
        return true;
    }

    bool ExtractFloatField(const std::string& text, const std::string& key, float& outValue)
    {
        std::size_t valueStart = 0U;
        if (!TryFindKeyValueStart(text, key, valueStart))
        {
            return false;
        }

        char* parseEnd = nullptr;
        const float value = std::strtof(text.c_str() + valueStart, &parseEnd);
        if (parseEnd == text.c_str() + valueStart)
        {
            return false;
        }

        outValue = value;
        return true;
    }

    bool ExtractNumberArrayField(const std::string& text, const std::string& key, std::vector<float>& outValues)
    {
        std::size_t valueStart = 0U;
        if (!TryFindKeyValueStart(text, key, valueStart) || text[valueStart] != '[')
        {
            outValues.clear();
            return false;
        }

        const std::size_t valueEnd = text.find(']', valueStart + 1U);
        if (valueEnd == std::string::npos)
        {
            outValues.clear();
            return false;
        }

        outValues.clear();
        std::string body = text.substr(valueStart + 1U, valueEnd - valueStart - 1U);
        std::size_t cursor = 0U;
        while (cursor < body.size())
        {
            while (cursor < body.size() &&
                (std::isspace(static_cast<unsigned char>(body[cursor])) != 0 || body[cursor] == ','))
            {
                ++cursor;
            }

            if (cursor >= body.size())
            {
                break;
            }

            char* parseEnd = nullptr;
            const float value = std::strtof(body.c_str() + cursor, &parseEnd);
            if (parseEnd == body.c_str() + cursor)
            {
                outValues.clear();
                return false;
            }

            outValues.push_back(value);
            cursor = static_cast<std::size_t>(parseEnd - body.c_str());
        }

        return !outValues.empty();
    }

    BuiltInMeshKind ParseBuiltInMeshKind(const std::string& value)
    {
        std::string normalized = value;
        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            });

        if (normalized == "triangle")
        {
            return BuiltInMeshKind::Triangle;
        }
        if (normalized == "diamond")
        {
            return BuiltInMeshKind::Diamond;
        }
        if (normalized == "cube")
        {
            return BuiltInMeshKind::Cube;
        }
        if (normalized == "quad")
        {
            return BuiltInMeshKind::Quad;
        }
        if (normalized == "octahedron")
        {
            return BuiltInMeshKind::Octahedron;
        }

        return BuiltInMeshKind::None;
    }

    Vector3 ToVector3(const std::vector<float>& values, const Vector3& fallback)
    {
        if (values.size() < 3U)
        {
            return fallback;
        }

        return Vector3(values[0], values[1], values[2]);
    }
}

std::unique_ptr<ImportedPrototypeData> PrototypeImporter::ImportFromJson(
    const std::string& sourceText,
    const std::string& assetId)
{
    auto importedData = std::make_unique<ImportedPrototypeData>();
    if (!ExtractStringField(sourceText, "type", importedData->type) || importedData->type != "object")
    {
        return nullptr;
    }

    if (!ExtractStringField(sourceText, "id", importedData->idText))
    {
        importedData->idText = assetId;
    }

    importedData->objectPrototype = ObjectPrototype(MakePrototypeId(importedData->idText));

    std::vector<float> values;
    if (ExtractNumberArrayField(sourceText, "position", values))
    {
        importedData->objectPrototype.transform.translation =
            ToVector3(values, importedData->objectPrototype.transform.translation);
    }
    if (ExtractNumberArrayField(sourceText, "rotation", values))
    {
        importedData->objectPrototype.transform.rotationRadians =
            ToVector3(values, importedData->objectPrototype.transform.rotationRadians);
    }
    if (ExtractNumberArrayField(sourceText, "scale", values))
    {
        importedData->objectPrototype.transform.scale =
            ToVector3(values, importedData->objectPrototype.transform.scale);
    }

    std::string meshKind;
    if (ExtractStringField(sourceText, "kind", meshKind))
    {
        importedData->objectPrototype.GetPrimaryMesh().builtInKind = ParseBuiltInMeshKind(meshKind);
    }

    if (ExtractNumberArrayField(sourceText, "baseColor", values) && values.size() >= 4U)
    {
        importedData->objectPrototype.GetPrimaryMesh().material.baseLayer.albedo =
            ColorPrototype(values[0], values[1], values[2], values[3]);
    }
    if (ExtractNumberArrayField(sourceText, "reflectionColor", values) && values.size() >= 4U)
    {
        importedData->objectPrototype.GetPrimaryMesh().material.reflectionColor =
            ColorPrototype(values[0], values[1], values[2], values[3]);
    }
    ExtractFloatField(sourceText, "reflectivity", importedData->objectPrototype.GetPrimaryMesh().material.reflectivity);
    ExtractFloatField(sourceText, "roughness", importedData->objectPrototype.GetPrimaryMesh().material.baseLayer.roughness);
    if (ExtractNumberArrayField(sourceText, "absorptionColor", values) && values.size() >= 4U)
    {
        importedData->objectPrototype.GetPrimaryMesh().material.absorptionColor =
            ColorPrototype(values[0], values[1], values[2], values[3]);
    }
    ExtractFloatField(sourceText, "absorption", importedData->objectPrototype.GetPrimaryMesh().material.absorption);

    ExtractStringField(sourceText, "role", importedData->objectPrototype.gameplayRole);
    ExtractIntField(sourceText, "scoreValue", importedData->objectPrototype.scoreValue);
    return importedData;
}
