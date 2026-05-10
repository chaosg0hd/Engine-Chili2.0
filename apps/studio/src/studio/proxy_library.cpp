#include "studio/proxy_library.hpp"
#include "runtime/studio_default_scene_template.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace studio
{
    namespace
    {
        constexpr const char* kProxyFolders[] = {
            "prototypes",
            "meshes",
            "materials",
            "textures",
            "scenes",
            "prefabs",
            "audio",
            "metadata"
        };

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

        void WriteFileIfMissing(const std::filesystem::path& filePath, const std::string& content)
        {
            std::error_code error;
            if (std::filesystem::exists(filePath, error))
            {
                return;
            }

            std::filesystem::create_directories(filePath.parent_path(), error);
            std::ofstream out(filePath);
            if (out.is_open())
            {
                out << content;
            }
        }
    }

    ProxyLibrary::ProxyLibrary(FileProxy files)
        : m_files(std::move(files))
    {
    }

    ProxyLibraryScanResult ProxyLibrary::ScanProjectLibrary(
        const std::string& projectRootPath,
        const std::string& assetProxyFolder) const
    {
        ProxyLibraryScanResult result;
        if (projectRootPath.empty())
        {
            result.message = "Project root path is empty.";
            return result;
        }

        const std::filesystem::path projectRoot = std::filesystem::path(m_files.Resolve(projectRootPath));
        std::filesystem::path proxyPath(assetProxyFolder);
        if (proxyPath.empty())
        {
            proxyPath = std::filesystem::path("../ChiliProxyLibrary");
        }
        if (!proxyPath.is_absolute())
        {
            proxyPath = (projectRoot / proxyPath).lexically_normal();
        }
        else
        {
            proxyPath = proxyPath.lexically_normal();
        }

        result.proxyFolderPath = NormalizeSlashes(proxyPath.string());
        EnsureProxyFolderStructure(result.proxyFolderPath);
        EnsureBootstrapLibraryContent(result.proxyFolderPath);

        for (const char* folderName : kProxyFolders)
        {
            ScanFolderEntries(result.proxyFolderPath, folderName, result.entries);
        }

        result.success = true;
        result.message = "Scanned proxy library.";
        return result;
    }

    bool ProxyLibrary::WriteProjectRegistry(
        const std::string& projectRootPath,
        const std::vector<ProxyLibraryEntry>& entries,
        std::string& outError) const
    {
        const std::string registryFolder = projectRootPath + "/.chili";
        if (!m_files.Exists(registryFolder) && !m_files.CreateDirectory(registryFolder, outError))
        {
            return false;
        }

        return m_files.WriteText(projectRootPath + "/.chili/asset_registry.json", BuildRegistryJson(entries), outError);
    }

    std::string ProxyLibrary::NormalizeSlashes(std::string value)
    {
        std::replace(value.begin(), value.end(), '\\', '/');
        return value;
    }

    std::string ProxyLibrary::EscapeJson(const std::string& value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (const char ch : value)
        {
            if (ch == '\\' || ch == '"')
            {
                escaped.push_back('\\');
            }
            escaped.push_back(ch);
        }
        return escaped;
    }

    std::string ProxyLibrary::BuildRegistryJson(const std::vector<ProxyLibraryEntry>& entries)
    {
        std::ostringstream json;
        json << "{\n  \"version\": 1,\n  \"entries\": [\n";
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            const ProxyLibraryEntry& entry = entries[i];
            json << "    {\n";
            json << "      \"id\": \"" << EscapeJson(entry.id) << "\",\n";
            json << "      \"type\": \"" << EscapeJson(entry.type) << "\",\n";
            json << "      \"name\": \"" << EscapeJson(entry.name) << "\",\n";
            json << "      \"sourcePath\": \"" << EscapeJson(entry.sourcePath) << "\",\n";
            json << "      \"libraryRelativePath\": \"" << EscapeJson(entry.libraryRelativePath) << "\",\n";
            json << "      \"version\": \"" << EscapeJson(entry.version) << "\",\n";
            json << "      \"tags\": [";
            for (std::size_t tagIndex = 0; tagIndex < entry.tags.size(); ++tagIndex)
            {
                if (tagIndex > 0U) json << ", ";
                json << "\"" << EscapeJson(entry.tags[tagIndex]) << "\"";
            }
            json << "]\n";
            json << "    }";
            if (i + 1U < entries.size()) json << ",";
            json << "\n";
        }
        json << "  ]\n}\n";
        return json.str();
    }

    std::string ProxyLibrary::ExtractJsonStringField(const std::string& text, const std::string& fieldName)
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

    std::vector<std::string> ProxyLibrary::ParseTags(const std::string& text)
    {
        std::vector<std::string> tags;
        const std::size_t keyPos = text.find("\"tags\"");
        if (keyPos == std::string::npos)
        {
            return tags;
        }
        const std::size_t open = text.find('[', keyPos);
        const std::size_t close = text.find(']', open);
        if (open == std::string::npos || close == std::string::npos || close <= open)
        {
            return tags;
        }

        std::size_t cursor = open + 1U;
        while (cursor < close)
        {
            const std::size_t firstQuote = text.find('"', cursor);
            if (firstQuote == std::string::npos || firstQuote >= close) break;
            const std::size_t secondQuote = text.find('"', firstQuote + 1U);
            if (secondQuote == std::string::npos || secondQuote > close) break;
            tags.push_back(text.substr(firstQuote + 1U, secondQuote - firstQuote - 1U));
            cursor = secondQuote + 1U;
        }
        return tags;
    }

    static std::vector<std::string> ParseStringArrayField(const std::string& text, const std::string& fieldName)
    {
        std::vector<std::string> values;
        const std::size_t keyPos = text.find("\"" + fieldName + "\"");
        if (keyPos == std::string::npos)
        {
            return values;
        }

        const std::size_t open = text.find('[', keyPos);
        const std::size_t close = text.find(']', open);
        if (open == std::string::npos || close == std::string::npos || close <= open)
        {
            return values;
        }

        std::size_t cursor = open + 1U;
        while (cursor < close)
        {
            const std::size_t firstQuote = text.find('"', cursor);
            if (firstQuote == std::string::npos || firstQuote >= close) break;
            const std::size_t secondQuote = text.find('"', firstQuote + 1U);
            if (secondQuote == std::string::npos || secondQuote > close) break;
            values.push_back(text.substr(firstQuote + 1U, secondQuote - firstQuote - 1U));
            cursor = secondQuote + 1U;
        }
        return values;
    }

    std::string ProxyLibrary::GuessTypeFromFolder(const std::string& folderName)
    {
        if (folderName == "prototypes") return "prototype";
        if (folderName == "meshes") return "mesh";
        if (folderName == "materials") return "material";
        if (folderName == "textures") return "texture";
        if (folderName == "scenes") return "scene";
        if (folderName == "prefabs") return "prefab";
        if (folderName == "audio") return "audio";
        return "asset";
    }

    std::string ProxyLibrary::GuessTypeFromExtension(const std::string& extension)
    {
        const std::string ext = extension;
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") return "texture";
        if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") return "audio";
        if (ext == ".scene" || ext == ".scene.json") return "scene";
        if (ext == ".mat.json" || ext == ".material.json") return "material";
        if (ext == ".mesh.json" || ext == ".obj" || ext == ".fbx") return "mesh";
        return "asset";
    }

    void ProxyLibrary::EnsureProxyFolderStructure(const std::string& proxyFolderPath)
    {
        std::error_code error;
        std::filesystem::create_directories(proxyFolderPath, error);
        for (const char* folderName : kProxyFolders)
        {
            std::filesystem::create_directories(std::filesystem::path(proxyFolderPath) / folderName, error);
        }
    }

    void ProxyLibrary::EnsureBootstrapLibraryContent(const std::string& proxyFolderPath)
    {
        const std::filesystem::path root(proxyFolderPath);
        WriteFileIfMissing(
            root / "prototypes" / "default_cube.prototype.json",
            "{\n"
            "  \"id\": \"proto.default_cube\",\n"
            "  \"name\": \"Default Cube\",\n"
            "  \"type\": \"StaticMeshObject\",\n"
            "  \"assets\": {\n"
            "    \"mesh\": \"builtin:cube\",\n"
            "    \"material\": \"builtin:default_material\"\n"
            "  },\n"
            "  \"defaults\": {\n"
            "    \"transform\": {\n"
            "      \"position\": [0, 0, 3],\n"
            "      \"rotation\": [0, 0, 0],\n"
            "      \"scale\": [1, 1, 1]\n"
            "    },\n"
            "    \"visible\": true\n"
            "  },\n"
            "  \"components\": [\"Name\", \"Transform\", \"Renderable\"],\n"
            "  \"tags\": [\"builtin\", \"default\", \"cube\"],\n"
            "  \"version\": \"1\"\n"
            "}\n");

        WriteFileIfMissing(
            root / "prototypes" / "default_light.prototype.json",
            "{\n"
            "  \"id\": \"proto.default_light\",\n"
            "  \"name\": \"Default Light\",\n"
            "  \"type\": \"PointLightObject\",\n"
            "  \"assets\": {\n"
            "    \"light\": \"builtin:point_light\"\n"
            "  },\n"
            "  \"defaults\": {\n"
            "    \"transform\": {\n"
            "      \"position\": [-2, 3, -1.5],\n"
            "      \"rotation\": [0, 0, 0],\n"
            "      \"scale\": [1, 1, 1]\n"
            "    },\n"
            "    \"visible\": true\n"
            "  },\n"
            "  \"components\": [\"Name\", \"Transform\", \"Light\"],\n"
            "  \"tags\": [\"builtin\", \"default\", \"light\"],\n"
            "  \"version\": \"1\"\n"
            "}\n");

        WriteFileIfMissing(
            root / "metadata" / "library.json",
            "{\n  \"name\": \"Chili Proxy Library\",\n  \"version\": \"1\"\n}\n");

        WriteFileIfMissing(
            root / "meshes" / "builtin_cube.asset.json",
            "{\n"
            "  \"id\": \"asset.mesh.builtin_cube\",\n"
            "  \"name\": \"Built-in Cube Mesh\",\n"
            "  \"type\": \"Mesh\",\n"
            "  \"source\": \"builtin:cube\",\n"
            "  \"tags\": [\"builtin\", \"cube\", \"mesh\"],\n"
            "  \"version\": \"1\"\n"
            "}\n");

        WriteFileIfMissing(
            root / "materials" / "default_material.asset.json",
            "{\n"
            "  \"id\": \"asset.material.default\",\n"
            "  \"name\": \"Default Material\",\n"
            "  \"type\": \"Material\",\n"
            "  \"source\": \"builtin:default_material\",\n"
            "  \"tags\": [\"builtin\", \"default\", \"material\"],\n"
            "  \"version\": \"1\"\n"
            "}\n");

        WriteFileIfMissing(
            root / "materials" / "point_light.asset.json",
            "{\n"
            "  \"id\": \"asset.light.point\",\n"
            "  \"name\": \"Point Light\",\n"
            "  \"type\": \"Light\",\n"
            "  \"source\": \"builtin:point_light\",\n"
            "  \"tags\": [\"builtin\", \"default\", \"light\"],\n"
            "  \"version\": \"1\"\n"
            "}\n");

        WriteFileIfMissing(
            root / "scenes" / "default.scene.json",
            studio_runtime::BuildDefaultSceneTemplateSceneJson());
    }

    void ProxyLibrary::ScanFolderEntries(
        const std::string& proxyFolderPath,
        const std::string& folderName,
        std::vector<ProxyLibraryEntry>& outEntries)
    {
        const std::filesystem::path folderPath = std::filesystem::path(proxyFolderPath) / folderName;
        std::error_code error;
        if (!std::filesystem::exists(folderPath, error))
        {
            return;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(folderPath, error))
        {
            if (error || !entry.is_regular_file())
            {
                continue;
            }

            const std::string absolutePath = NormalizeSlashes(entry.path().string());
            const std::string relativePath = NormalizeSlashes((std::filesystem::path(folderName) / entry.path().filename()).string());
            const std::string extension = entry.path().extension().string();
            if (folderName == "prototypes" && extension == ".json")
            {
                outEntries.push_back(BuildEntryFromPrototypeFile(absolutePath, relativePath));
            }
            else
            {
                outEntries.push_back(BuildEntryFromGenericFile(absolutePath, relativePath, GuessTypeFromFolder(folderName)));
            }
        }
    }

    ProxyLibraryEntry ProxyLibrary::BuildEntryFromPrototypeFile(
        const std::string& filePath,
        const std::string& libraryRelativePath)
    {
        ProxyLibraryEntry entry;
        entry.type = "prototype";
        entry.sourcePath = filePath;
        entry.libraryRelativePath = libraryRelativePath;
        entry.version = "1";

        const std::string text = ReadFileText(filePath);
        entry.id = ExtractJsonStringField(text, "id");
        entry.name = ExtractJsonStringField(text, "name");
        entry.tags = ParseTags(text);

        if (entry.id.empty())
        {
            entry.id = "proto." + NormalizeSlashes(std::filesystem::path(filePath).stem().string());
        }
        if (entry.name.empty())
        {
            entry.name = std::filesystem::path(filePath).stem().string();
        }
        return entry;
    }

    ProxyLibraryEntry ProxyLibrary::BuildEntryFromGenericFile(
        const std::string& filePath,
        const std::string& libraryRelativePath,
        const std::string& type)
    {
        ProxyLibraryEntry entry;
        entry.type = type;
        entry.sourcePath = filePath;
        entry.libraryRelativePath = libraryRelativePath;
        entry.version = "1";

        const std::filesystem::path path(filePath);
        const std::string text = ReadFileText(filePath);
        entry.id = ExtractJsonStringField(text, "id");
        entry.name = ExtractJsonStringField(text, "name");
        entry.tags = ParseTags(text);

        const std::string declaredVersion = ExtractJsonStringField(text, "version");
        if (!declaredVersion.empty())
        {
            entry.version = declaredVersion;
        }

        if (entry.name.empty())
        {
            entry.name = path.stem().string();
        }
        if (entry.id.empty())
        {
            entry.id = "asset." + type + "." + NormalizeSlashes(path.stem().string());
        }

        if (entry.type == "asset")
        {
            entry.type = GuessTypeFromExtension(path.extension().string());
            if (entry.id.empty())
            {
                entry.id = "asset." + entry.type + "." + NormalizeSlashes(path.stem().string());
            }
        }
        return entry;
    }

    bool ProxyLibrary::GetPrototypeInfoById(
        const std::string& proxyFolderPath,
        const std::string& prototypeId,
        ProxyPrototypeInfo& outInfo) const
    {
        outInfo = ProxyPrototypeInfo{};
        if (prototypeId.empty() || proxyFolderPath.empty())
        {
            return false;
        }

        std::error_code error;
        const std::filesystem::path folder = std::filesystem::path(proxyFolderPath) / "prototypes";
        if (!std::filesystem::exists(folder, error))
        {
            return false;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(folder, error))
        {
            if (error || !entry.is_regular_file())
            {
                continue;
            }

            const std::string sourcePath = NormalizeSlashes(entry.path().string());
            const std::string text = ReadFileText(sourcePath);
            if (ExtractJsonStringField(text, "id") != prototypeId)
            {
                continue;
            }

            outInfo.found = true;
            outInfo.id = prototypeId;
            outInfo.name = ExtractJsonStringField(text, "name");
            outInfo.type = ExtractJsonStringField(text, "type");
            outInfo.meshAsset = ExtractJsonStringField(text, "mesh");
            outInfo.materialAsset = ExtractJsonStringField(text, "material");
            outInfo.lightAsset = ExtractJsonStringField(text, "light");
            outInfo.sourcePath = sourcePath;
            outInfo.components = ParseStringArrayField(text, "components");
            return true;
        }

        return false;
    }
}
