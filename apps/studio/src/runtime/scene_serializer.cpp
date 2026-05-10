#include "runtime/scene_serializer.hpp"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace studio_runtime
{
    namespace
    {
        class JsonCursor
        {
        public:
            explicit JsonCursor(const std::string& text)
                : m_text(text)
            {
            }

            bool ReadObjectBegin() { return ReadChar('{'); }
            bool ReadObjectEnd() { return ReadChar('}'); }
            bool ReadArrayBegin() { return ReadChar('['); }
            bool ReadArrayEnd() { return ReadChar(']'); }
            bool ReadColon() { return ReadChar(':'); }
            bool ReadComma()
            {
                SkipWhitespace();
                if (Peek() == ',')
                {
                    ++m_pos;
                    return true;
                }
                return false;
            }

            bool ReadString(std::string& out)
            {
                SkipWhitespace();
                if (Peek() != '"')
                {
                    return false;
                }

                ++m_pos;
                out.clear();
                while (m_pos < m_text.size())
                {
                    const char ch = m_text[m_pos++];
                    if (ch == '"')
                    {
                        return true;
                    }

                    if (ch == '\\' && m_pos < m_text.size())
                    {
                        out.push_back(m_text[m_pos++]);
                        continue;
                    }

                    out.push_back(ch);
                }

                return false;
            }

            bool ReadNumber(float& out)
            {
                SkipWhitespace();
                const std::size_t start = m_pos;
                if (Peek() == '-')
                {
                    ++m_pos;
                }
                while (std::isdigit(static_cast<unsigned char>(Peek())) != 0)
                {
                    ++m_pos;
                }
                if (Peek() == '.')
                {
                    ++m_pos;
                    while (std::isdigit(static_cast<unsigned char>(Peek())) != 0)
                    {
                        ++m_pos;
                    }
                }

                if (start == m_pos)
                {
                    return false;
                }

                out = std::stof(m_text.substr(start, m_pos - start));
                return true;
            }

            bool ReadInteger(EntityId& out)
            {
                float value = 0.0f;
                if (!ReadNumber(value))
                {
                    return false;
                }

                out = static_cast<EntityId>(value);
                return true;
            }

            bool ReadBool(bool& out)
            {
                SkipWhitespace();
                if (m_text.compare(m_pos, 4U, "true") == 0)
                {
                    m_pos += 4U;
                    out = true;
                    return true;
                }
                if (m_text.compare(m_pos, 5U, "false") == 0)
                {
                    m_pos += 5U;
                    out = false;
                    return true;
                }
                return false;
            }

            bool SkipValue()
            {
                SkipWhitespace();
                const char ch = Peek();
                if (ch == '{')
                {
                    ++m_pos;
                    int depth = 1;
                    while (m_pos < m_text.size() && depth > 0)
                    {
                        const char next = m_text[m_pos++];
                        if (next == '{') ++depth;
                        if (next == '}') --depth;
                    }
                    return depth == 0;
                }
                if (ch == '[')
                {
                    ++m_pos;
                    int depth = 1;
                    while (m_pos < m_text.size() && depth > 0)
                    {
                        const char next = m_text[m_pos++];
                        if (next == '[') ++depth;
                        if (next == ']') --depth;
                    }
                    return depth == 0;
                }
                if (ch == '"')
                {
                    std::string ignored;
                    return ReadString(ignored);
                }
                while (m_pos < m_text.size() && Peek() != ',' && Peek() != '}' && Peek() != ']')
                {
                    ++m_pos;
                }
                return true;
            }

            char Peek()
            {
                SkipWhitespace();
                return m_pos < m_text.size() ? m_text[m_pos] : '\0';
            }

        private:
            bool ReadChar(char expected)
            {
                SkipWhitespace();
                if (Peek() != expected)
                {
                    return false;
                }
                ++m_pos;
                return true;
            }

            void SkipWhitespace()
            {
                while (m_pos < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_pos])) != 0)
                {
                    ++m_pos;
                }
            }

        private:
            const std::string& m_text;
            std::size_t m_pos = 0;
        };

        std::string EscapeJson(const std::string& value)
        {
            std::string escaped;
            for (const char ch : value)
            {
                if (ch == '"' || ch == '\\')
                {
                    escaped.push_back('\\');
                }
                escaped.push_back(ch);
            }
            return escaped;
        }

        bool ReadVector3(JsonCursor& cursor, Vector3& out)
        {
            if (!cursor.ReadArrayBegin())
            {
                return false;
            }
            if (!cursor.ReadNumber(out.x)) return false;
            cursor.ReadComma();
            if (!cursor.ReadNumber(out.y)) return false;
            cursor.ReadComma();
            if (!cursor.ReadNumber(out.z)) return false;
            return cursor.ReadArrayEnd();
        }

        bool ReadColor(JsonCursor& cursor, ColorPrototype& out)
        {
            Vector3 rgb;
            if (!ReadVector3(cursor, rgb))
            {
                return false;
            }
            out = ColorPrototype(rgb.x, rgb.y, rgb.z, 1.0f);
            return true;
        }

        BuiltInMeshKind MeshFromString(const std::string& value)
        {
            if (value == "builtin:cube") return BuiltInMeshKind::Cube;
            if (value == "builtin:quad") return BuiltInMeshKind::Quad;
            if (value == "builtin:octahedron") return BuiltInMeshKind::Octahedron;
            if (value == "builtin:diamond") return BuiltInMeshKind::Diamond;
            if (value == "builtin:triangle") return BuiltInMeshKind::Triangle;
            return BuiltInMeshKind::None;
        }

        const char* MeshToString(BuiltInMeshKind mesh)
        {
            switch (mesh)
            {
            case BuiltInMeshKind::Cube: return "builtin:cube";
            case BuiltInMeshKind::Quad: return "builtin:quad";
            case BuiltInMeshKind::Octahedron: return "builtin:octahedron";
            case BuiltInMeshKind::Diamond: return "builtin:diamond";
            case BuiltInMeshKind::Triangle: return "builtin:triangle";
            case BuiltInMeshKind::None:
            default:
                return "builtin:none";
            }
        }

        bool ReadTransform(JsonCursor& cursor, TransformPrototype& transform)
        {
            if (!cursor.ReadObjectBegin())
            {
                return false;
            }

            while (cursor.Peek() != '}')
            {
                std::string key;
                if (!cursor.ReadString(key) || !cursor.ReadColon())
                {
                    return false;
                }

                if (key == "position")
                {
                    if (!ReadVector3(cursor, transform.translation)) return false;
                }
                else if (key == "rotation")
                {
                    if (!ReadVector3(cursor, transform.rotationRadians)) return false;
                }
                else if (key == "scale")
                {
                    if (!ReadVector3(cursor, transform.scale)) return false;
                }
                else
                {
                    cursor.SkipValue();
                }

                cursor.ReadComma();
            }

            return cursor.ReadObjectEnd();
        }

        bool ReadRenderable(JsonCursor& cursor, RenderableComponent& renderable)
        {
            if (!cursor.ReadObjectBegin())
            {
                return false;
            }

            while (cursor.Peek() != '}')
            {
                std::string key;
                if (!cursor.ReadString(key) || !cursor.ReadColon())
                {
                    return false;
                }

                if (key == "mesh")
                {
                    std::string mesh;
                    if (!cursor.ReadString(mesh)) return false;
                    renderable.mesh = MeshFromString(mesh);
                }
                else if (key == "visible")
                {
                    if (!cursor.ReadBool(renderable.visible)) return false;
                }
                else if (key == "albedo")
                {
                    if (!ReadColor(cursor, renderable.material.baseLayer.albedo)) return false;
                }
                else
                {
                    cursor.SkipValue();
                }

                cursor.ReadComma();
            }

            return cursor.ReadObjectEnd();
        }

        bool ReadObject(JsonCursor& cursor, ObjectComponent& object)
        {
            if (!cursor.ReadObjectBegin())
            {
                return false;
            }

            while (cursor.Peek() != '}')
            {
                std::string key;
                if (!cursor.ReadString(key) || !cursor.ReadColon())
                {
                    return false;
                }

                if (key == "kind")
                {
                    if (!cursor.ReadString(object.kind)) return false;
                }
                else if (key == "prototype")
                {
                    if (!cursor.ReadString(object.prototypeId)) return false;
                }
                else if (key == "selectable")
                {
                    if (!cursor.ReadBool(object.selectable)) return false;
                }
                else
                {
                    cursor.SkipValue();
                }

                cursor.ReadComma();
            }

            return cursor.ReadObjectEnd();
        }

        bool ReadLight(JsonCursor& cursor, LightComponent& light)
        {
            if (!cursor.ReadObjectBegin())
            {
                return false;
            }

            light.light.emitter.kind = LightEmitterKind::Point;
            light.light.emitter.range = 24.0f;
            light.light.emitter.intensity = 3.5f;
            light.light.enabled = true;

            while (cursor.Peek() != '}')
            {
                std::string key;
                if (!cursor.ReadString(key) || !cursor.ReadColon())
                {
                    return false;
                }

                if (key == "position")
                {
                    if (!ReadVector3(cursor, light.light.emitter.position)) return false;
                }
                else if (key == "color")
                {
                    if (!ReadColor(cursor, light.light.emitter.color)) return false;
                }
                else if (key == "intensity")
                {
                    if (!cursor.ReadNumber(light.light.emitter.intensity)) return false;
                }
                else if (key == "range")
                {
                    if (!cursor.ReadNumber(light.light.emitter.range)) return false;
                }
                else if (key == "enabled")
                {
                    if (!cursor.ReadBool(light.light.enabled)) return false;
                }
                else
                {
                    cursor.SkipValue();
                }

                cursor.ReadComma();
            }

            return cursor.ReadObjectEnd();
        }

        bool ReadEntity(JsonCursor& cursor, RuntimeWorld& world)
        {
            if (!cursor.ReadObjectBegin())
            {
                return false;
            }

            EntityId id = 0;
            std::string name = "Entity";
            TransformPrototype transform;
            ObjectComponent object;
            RenderableComponent renderable;
            LightComponent light;
            bool hasObject = false;
            bool hasRenderable = false;
            bool hasLight = false;
            std::string rootPrototypeId;

            while (cursor.Peek() != '}')
            {
                std::string key;
                if (!cursor.ReadString(key) || !cursor.ReadColon())
                {
                    return false;
                }

                if (key == "id")
                {
                    if (!cursor.ReadInteger(id)) return false;
                }
                else if (key == "name")
                {
                    if (!cursor.ReadString(name)) return false;
                }
                else if (key == "prototype")
                {
                    if (!cursor.ReadString(rootPrototypeId)) return false;
                }
                else if (key == "transform")
                {
                    if (!ReadTransform(cursor, transform)) return false;
                }
                else if (key == "values")
                {
                    if (!cursor.ReadObjectBegin()) return false;
                    while (cursor.Peek() != '}')
                    {
                        std::string valuesKey;
                        if (!cursor.ReadString(valuesKey) || !cursor.ReadColon())
                        {
                            return false;
                        }

                        if (valuesKey == "transform")
                        {
                            if (!ReadTransform(cursor, transform)) return false;
                        }
                        else
                        {
                            cursor.SkipValue();
                        }

                        cursor.ReadComma();
                    }
                    if (!cursor.ReadObjectEnd()) return false;
                }
                else if (key == "overrides")
                {
                    if (!cursor.ReadObjectBegin()) return false;
                    while (cursor.Peek() != '}')
                    {
                        std::string overridesKey;
                        if (!cursor.ReadString(overridesKey) || !cursor.ReadColon())
                        {
                            return false;
                        }

                        if (overridesKey == "object")
                        {
                            if (!cursor.ReadObjectBegin()) return false;
                            while (cursor.Peek() != '}')
                            {
                                std::string objectKey;
                                if (!cursor.ReadString(objectKey) || !cursor.ReadColon())
                                {
                                    return false;
                                }

                                if (objectKey == "selectable")
                                {
                                    if (!cursor.ReadBool(object.selectable)) return false;
                                    hasObject = true;
                                }
                                else if (objectKey == "kind")
                                {
                                    if (!cursor.ReadString(object.kind)) return false;
                                    hasObject = true;
                                }
                                else
                                {
                                    cursor.SkipValue();
                                }
                                cursor.ReadComma();
                            }
                            if (!cursor.ReadObjectEnd()) return false;
                        }
                        else if (overridesKey == "renderable")
                        {
                            if (!cursor.ReadObjectBegin()) return false;
                            while (cursor.Peek() != '}')
                            {
                                std::string renderableKey;
                                if (!cursor.ReadString(renderableKey) || !cursor.ReadColon())
                                {
                                    return false;
                                }

                                if (renderableKey == "visible")
                                {
                                    if (!cursor.ReadBool(renderable.visible)) return false;
                                    hasRenderable = true;
                                }
                                else if (renderableKey == "mesh")
                                {
                                    std::string mesh;
                                    if (!cursor.ReadString(mesh)) return false;
                                    renderable.mesh = MeshFromString(mesh);
                                    hasRenderable = true;
                                }
                                else
                                {
                                    cursor.SkipValue();
                                }
                                cursor.ReadComma();
                            }
                            if (!cursor.ReadObjectEnd()) return false;
                        }
                        else if (overridesKey == "light")
                        {
                            if (!ReadLight(cursor, light)) return false;
                            hasLight = true;
                        }
                        else
                        {
                            cursor.SkipValue();
                        }

                        cursor.ReadComma();
                    }
                    if (!cursor.ReadObjectEnd()) return false;
                }
                else if (key == "object")
                {
                    if (!ReadObject(cursor, object)) return false;
                    hasObject = true;
                }
                else if (key == "renderable")
                {
                    if (!ReadRenderable(cursor, renderable)) return false;
                    hasRenderable = true;
                }
                else if (key == "light")
                {
                    if (!ReadLight(cursor, light)) return false;
                    hasLight = true;
                }
                else
                {
                    cursor.SkipValue();
                }

                cursor.ReadComma();
            }

            if (!cursor.ReadObjectEnd())
            {
                return false;
            }

            const std::string resolvedPrototypeId =
                !object.prototypeId.empty() ? object.prototypeId : rootPrototypeId;
            if (!resolvedPrototypeId.empty())
            {
                object.prototypeId = resolvedPrototypeId;
                hasObject = true;
            }

            const EntityId entity = world.CreateEntityWithId(id, name);
            if (entity == 0)
            {
                return false;
            }
            world.SetTransform(entity, transform);
            if (hasObject)
            {
                world.SetObject(entity, object);
            }
            if (hasRenderable)
            {
                world.SetRenderable(entity, renderable);
            }
            if (hasLight)
            {
                world.SetLight(entity, light);
            }
            return true;
        }
    }

    bool SceneSerializer::LoadFromFile(const std::string& path, RuntimeWorld& world, std::string& outError)
    {
        std::ifstream stream(path);
        if (!stream.is_open())
        {
            outError = "Could not open scene file: " + path;
            return false;
        }

        std::stringstream buffer;
        buffer << stream.rdbuf();
        return LoadFromText(buffer.str(), world, outError);
    }

    bool SceneSerializer::SaveToFile(const std::string& path, const RuntimeWorld& world, std::string& outError)
    {
        std::ofstream stream(path);
        if (!stream.is_open())
        {
            outError = "Could not write scene file: " + path;
            return false;
        }

        stream << SaveToText(world);
        return true;
    }

    bool SceneSerializer::LoadFromText(const std::string& text, RuntimeWorld& world, std::string& outError)
    {
        RuntimeWorld loaded;
        JsonCursor cursor(text);
        if (!cursor.ReadObjectBegin())
        {
            outError = "Scene JSON root must be an object.";
            return false;
        }

        bool readEntities = false;
        while (cursor.Peek() != '}')
        {
            std::string key;
            if (!cursor.ReadString(key) || !cursor.ReadColon())
            {
                outError = "Scene JSON contains an invalid property.";
                return false;
            }

            if (key == "entities")
            {
                if (!cursor.ReadArrayBegin())
                {
                    outError = "Scene entities must be an array.";
                    return false;
                }

                while (cursor.Peek() != ']')
                {
                    if (!ReadEntity(cursor, loaded))
                    {
                        outError = "Failed to read scene entity.";
                        return false;
                    }
                    cursor.ReadComma();
                }

                if (!cursor.ReadArrayEnd())
                {
                    outError = "Scene entities array was not closed.";
                    return false;
                }
                readEntities = true;
            }
            else
            {
                cursor.SkipValue();
            }

            cursor.ReadComma();
        }

        if (!cursor.ReadObjectEnd() || !readEntities)
        {
            outError = "Scene JSON did not contain entities.";
            return false;
        }

        world = loaded;
        return true;
    }

    std::string SceneSerializer::SaveToText(const RuntimeWorld& world)
    {
        std::ostringstream out;
        out << "{\n  \"version\": 1,\n  \"entities\": [\n";
        const std::vector<EntityId> ids = world.GetEntityList();
        for (std::size_t index = 0; index < ids.size(); ++index)
        {
            EntityInfo info;
            world.GetEntityInfo(ids[index], info);
            const bool hasPrototype = info.hasObject && !info.object.prototypeId.empty();
            out << "    {\n";
            out << "      \"id\": " << info.id << ",\n";
            out << "      \"name\": \"" << EscapeJson(info.name) << "\",\n";
            if (hasPrototype)
            {
                out << "      \"prototype\": \"" << EscapeJson(info.object.prototypeId) << "\",\n";
            }
            out << "      \"values\": {\n";
            out << "        \"transform\": {\n";
            out << "          \"position\": [" << info.transform.translation.x << ", " << info.transform.translation.y << ", " << info.transform.translation.z << "],\n";
            out << "          \"rotation\": [" << info.transform.rotationRadians.x << ", " << info.transform.rotationRadians.y << ", " << info.transform.rotationRadians.z << "],\n";
            out << "          \"scale\": [" << info.transform.scale.x << ", " << info.transform.scale.y << ", " << info.transform.scale.z << "]\n";
            out << "        }\n";
            out << "      },\n";
            bool wroteAnyOverride = false;
            std::ostringstream overrides;
            if (info.hasObject && (!info.object.selectable || (info.object.kind != "Object" && info.object.kind != "Light")))
            {
                if (wroteAnyOverride) overrides << ",\n";
                overrides << "        \"object\": {\n";
                overrides << "          \"kind\": \"" << EscapeJson(info.object.kind) << "\",\n";
                overrides << "          \"selectable\": " << (info.object.selectable ? "true" : "false") << "\n";
                overrides << "        }";
                wroteAnyOverride = true;
            }
            if (info.hasRenderable && hasPrototype && !info.renderable.visible)
            {
                if (wroteAnyOverride) overrides << ",\n";
                overrides << "        \"renderable\": {\n";
                overrides << "          \"visible\": false\n";
                overrides << "        }";
                wroteAnyOverride = true;
            }
            if (info.hasLight && hasPrototype)
            {
                const ColorPrototype color = info.light.light.emitter.color;
                const Vector3 position = info.light.light.emitter.position;
                if (wroteAnyOverride) overrides << ",\n";
                overrides << "        \"light\": {\n";
                overrides << "          \"position\": [" << position.x << ", " << position.y << ", " << position.z << "],\n";
                overrides << "          \"color\": [" << color.r << ", " << color.g << ", " << color.b << "],\n";
                overrides << "          \"intensity\": " << info.light.light.emitter.intensity << ",\n";
                overrides << "          \"range\": " << info.light.light.emitter.range << ",\n";
                overrides << "          \"enabled\": " << (info.light.light.enabled ? "true" : "false") << "\n";
                overrides << "        }";
                wroteAnyOverride = true;
            }
            out << "      \"overrides\": {";
            if (wroteAnyOverride)
            {
                out << "\n" << overrides.str() << "\n      ";
            }
            out << "}";

            if (info.hasRenderable && !hasPrototype)
            {
                const ColorPrototype color = info.renderable.material.baseLayer.albedo;
                out << ",\n      \"renderable\": {\n";
                out << "        \"mesh\": \"" << MeshToString(info.renderable.mesh) << "\",\n";
                out << "        \"visible\": " << (info.renderable.visible ? "true" : "false") << ",\n";
                out << "        \"albedo\": [" << color.r << ", " << color.g << ", " << color.b << "]\n";
                out << "      }";
            }
            if (info.hasLight && !hasPrototype)
            {
                const ColorPrototype color = info.light.light.emitter.color;
                const Vector3 position = info.light.light.emitter.position;
                out << ",\n      \"light\": {\n";
                out << "        \"position\": [" << position.x << ", " << position.y << ", " << position.z << "],\n";
                out << "        \"color\": [" << color.r << ", " << color.g << ", " << color.b << "],\n";
                out << "        \"intensity\": " << info.light.light.emitter.intensity << ",\n";
                out << "        \"range\": " << info.light.light.emitter.range << ",\n";
                out << "        \"enabled\": " << (info.light.light.enabled ? "true" : "false") << "\n";
                out << "      }";
            }
            out << "\n    }" << (index + 1U < ids.size() ? "," : "") << "\n";
        }
        out << "  ]\n}\n";
        return out.str();
    }
}
