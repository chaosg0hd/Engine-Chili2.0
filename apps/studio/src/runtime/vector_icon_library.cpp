#include "runtime/vector_icon_library.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace studio_runtime
{
    namespace
    {
        struct Cursor
        {
            const std::string& text;
            std::size_t index = 0;
        };

        void SkipWhitespace(Cursor& cursor)
        {
            while (cursor.index < cursor.text.size() && std::isspace(static_cast<unsigned char>(cursor.text[cursor.index])))
            {
                ++cursor.index;
            }
        }

        bool Consume(Cursor& cursor, char expected)
        {
            SkipWhitespace(cursor);
            if (cursor.index >= cursor.text.size() || cursor.text[cursor.index] != expected)
            {
                return false;
            }
            ++cursor.index;
            return true;
        }

        bool ParseString(Cursor& cursor, std::string& outValue)
        {
            SkipWhitespace(cursor);
            if (cursor.index >= cursor.text.size() || cursor.text[cursor.index] != '"')
            {
                return false;
            }

            ++cursor.index;
            outValue.clear();
            while (cursor.index < cursor.text.size())
            {
                const char ch = cursor.text[cursor.index++];
                if (ch == '"')
                {
                    return true;
                }
                if (ch == '\\' && cursor.index < cursor.text.size())
                {
                    outValue.push_back(cursor.text[cursor.index++]);
                    continue;
                }
                outValue.push_back(ch);
            }

            return false;
        }

        bool ParseFloat(Cursor& cursor, float& outValue)
        {
            SkipWhitespace(cursor);
            const std::size_t start = cursor.index;
            if (cursor.index < cursor.text.size() &&
                (cursor.text[cursor.index] == '-' || cursor.text[cursor.index] == '+'))
            {
                ++cursor.index;
            }
            while (cursor.index < cursor.text.size() &&
                (std::isdigit(static_cast<unsigned char>(cursor.text[cursor.index])) || cursor.text[cursor.index] == '.'))
            {
                ++cursor.index;
            }

            if (start == cursor.index)
            {
                return false;
            }

            outValue = std::strtof(cursor.text.c_str() + start, nullptr);
            return true;
        }

        bool ParseFloatArray2(Cursor& cursor, float& a, float& b)
        {
            if (!Consume(cursor, '[')) return false;
            if (!ParseFloat(cursor, a)) return false;
            if (!Consume(cursor, ',')) return false;
            if (!ParseFloat(cursor, b)) return false;
            if (!Consume(cursor, ']')) return false;
            return true;
        }

        bool ParseViewBox(Cursor& cursor, float& x, float& y, float& w, float& h)
        {
            if (!Consume(cursor, '[')) return false;
            if (!ParseFloat(cursor, x)) return false;
            if (!Consume(cursor, ',')) return false;
            if (!ParseFloat(cursor, y)) return false;
            if (!Consume(cursor, ',')) return false;
            if (!ParseFloat(cursor, w)) return false;
            if (!Consume(cursor, ',')) return false;
            if (!ParseFloat(cursor, h)) return false;
            if (!Consume(cursor, ']')) return false;
            return true;
        }

        bool ParsePathData(const std::string& text, VectorIconCommand& outCommand)
        {
            outCommand.pathPoints.clear();
            outCommand.closePath = false;

            std::size_t index = 0;
            char mode = 0;
            while (index < text.size())
            {
                while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index])))
                {
                    ++index;
                }
                if (index >= text.size())
                {
                    break;
                }

                const char token = text[index];
                if (token == 'M' || token == 'L' || token == 'Z')
                {
                    mode = token;
                    ++index;
                    if (mode == 'Z')
                    {
                        outCommand.closePath = true;
                    }
                    continue;
                }

                if (mode != 'M' && mode != 'L')
                {
                    return false;
                }

                char* endX = nullptr;
                const float x = std::strtof(text.c_str() + index, &endX);
                if (endX == text.c_str() + index)
                {
                    return false;
                }
                index = static_cast<std::size_t>(endX - text.c_str());
                while (index < text.size() && (std::isspace(static_cast<unsigned char>(text[index])) || text[index] == ','))
                {
                    ++index;
                }
                char* endY = nullptr;
                const float y = std::strtof(text.c_str() + index, &endY);
                if (endY == text.c_str() + index)
                {
                    return false;
                }
                index = static_cast<std::size_t>(endY - text.c_str());
                outCommand.pathPoints.push_back(VectorIconPoint{ x, y });
            }

            return outCommand.pathPoints.size() >= 2;
        }

        bool SkipValue(Cursor& cursor)
        {
            SkipWhitespace(cursor);
            if (cursor.index >= cursor.text.size())
            {
                return false;
            }

            const char ch = cursor.text[cursor.index];
            if (ch == '{')
            {
                int depth = 0;
                do
                {
                    if (cursor.text[cursor.index] == '{') ++depth;
                    else if (cursor.text[cursor.index] == '}') --depth;
                    ++cursor.index;
                } while (cursor.index < cursor.text.size() && depth > 0);
                return depth == 0;
            }
            if (ch == '[')
            {
                int depth = 0;
                do
                {
                    if (cursor.text[cursor.index] == '[') ++depth;
                    else if (cursor.text[cursor.index] == ']') --depth;
                    ++cursor.index;
                } while (cursor.index < cursor.text.size() && depth > 0);
                return depth == 0;
            }
            if (ch == '"')
            {
                std::string dummy;
                return ParseString(cursor, dummy);
            }

            while (cursor.index < cursor.text.size() &&
                cursor.text[cursor.index] != ',' &&
                cursor.text[cursor.index] != '}' &&
                cursor.text[cursor.index] != ']')
            {
                ++cursor.index;
            }
            return true;
        }

        bool ParseCommand(Cursor& cursor, VectorIconCommand& outCommand)
        {
            if (!Consume(cursor, '{'))
            {
                return false;
            }

            outCommand = VectorIconCommand{};
            std::string typeValue;
            std::string pathData;

            while (true)
            {
                SkipWhitespace(cursor);
                if (Consume(cursor, '}'))
                {
                    break;
                }

                std::string key;
                if (!ParseString(cursor, key) || !Consume(cursor, ':'))
                {
                    return false;
                }

                if (key == "type")
                {
                    if (!ParseString(cursor, typeValue)) return false;
                }
                else if (key == "from")
                {
                    if (!ParseFloatArray2(cursor, outCommand.from.x, outCommand.from.y)) return false;
                }
                else if (key == "to")
                {
                    if (!ParseFloatArray2(cursor, outCommand.to.x, outCommand.to.y)) return false;
                }
                else if (key == "center")
                {
                    if (!ParseFloatArray2(cursor, outCommand.center.x, outCommand.center.y)) return false;
                }
                else if (key == "radius")
                {
                    if (!ParseFloat(cursor, outCommand.radius)) return false;
                }
                else if (key == "d")
                {
                    if (!ParseString(cursor, pathData)) return false;
                }
                else
                {
                    if (!SkipValue(cursor)) return false;
                }

                SkipWhitespace(cursor);
                if (Consume(cursor, ','))
                {
                    continue;
                }
            }

            if (typeValue == "line")
            {
                outCommand.type = VectorIconCommandType::Line;
                return true;
            }
            if (typeValue == "circle")
            {
                outCommand.type = VectorIconCommandType::Circle;
                return true;
            }
            if (typeValue == "path")
            {
                outCommand.type = VectorIconCommandType::Path;
                return ParsePathData(pathData, outCommand);
            }

            return false;
        }

        bool ParseCommands(Cursor& cursor, std::vector<VectorIconCommand>& outCommands)
        {
            if (!Consume(cursor, '['))
            {
                return false;
            }

            outCommands.clear();
            while (true)
            {
                SkipWhitespace(cursor);
                if (Consume(cursor, ']'))
                {
                    break;
                }

                VectorIconCommand command;
                if (!ParseCommand(cursor, command))
                {
                    return false;
                }
                outCommands.push_back(std::move(command));

                SkipWhitespace(cursor);
                if (Consume(cursor, ','))
                {
                    continue;
                }
            }
            return true;
        }

        bool ParseIcon(Cursor& cursor, VectorIcon& outIcon)
        {
            if (!Consume(cursor, '{'))
            {
                return false;
            }

            outIcon = VectorIcon{};

            while (true)
            {
                SkipWhitespace(cursor);
                if (Consume(cursor, '}'))
                {
                    break;
                }

                std::string key;
                if (!ParseString(cursor, key) || !Consume(cursor, ':'))
                {
                    return false;
                }

                if (key == "id")
                {
                    if (!ParseString(cursor, outIcon.id)) return false;
                }
                else if (key == "viewBox")
                {
                    if (!ParseViewBox(cursor, outIcon.viewBoxMinX, outIcon.viewBoxMinY, outIcon.viewBoxWidth, outIcon.viewBoxHeight))
                    {
                        return false;
                    }
                }
                else if (key == "hotspot")
                {
                    if (!ParseFloatArray2(cursor, outIcon.hotspot.x, outIcon.hotspot.y)) return false;
                }
                else if (key == "commands")
                {
                    if (!ParseCommands(cursor, outIcon.commands)) return false;
                }
                else
                {
                    if (!SkipValue(cursor)) return false;
                }

                SkipWhitespace(cursor);
                if (Consume(cursor, ','))
                {
                    continue;
                }
            }

            return !outIcon.id.empty() && !outIcon.commands.empty();
        }
    }

    bool VectorIconLibrary::LoadFromFile(const std::string& path, std::string& outError)
    {
        std::ifstream input(path, std::ios::in | std::ios::binary);
        if (!input.is_open())
        {
            outError = "Unable to open vector icon JSON file: " + path;
            return false;
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        return LoadFromJson(buffer.str(), outError);
    }

    bool VectorIconLibrary::LoadFromJson(const std::string& json, std::string& outError)
    {
        Cursor cursor{ json, 0 };
        if (!Consume(cursor, '{'))
        {
            outError = "Vector icon JSON must begin with an object.";
            return false;
        }

        std::vector<VectorIcon> parsed;

        while (true)
        {
            SkipWhitespace(cursor);
            if (Consume(cursor, '}'))
            {
                break;
            }

            std::string key;
            if (!ParseString(cursor, key) || !Consume(cursor, ':'))
            {
                outError = "Vector icon JSON key/value parse error.";
                return false;
            }

            if (key == "icons")
            {
                if (!Consume(cursor, '['))
                {
                    outError = "Vector icon JSON 'icons' must be an array.";
                    return false;
                }

                while (true)
                {
                    SkipWhitespace(cursor);
                    if (Consume(cursor, ']'))
                    {
                        break;
                    }

                    VectorIcon icon;
                    if (!ParseIcon(cursor, icon))
                    {
                        outError = "Vector icon JSON icon parse error.";
                        return false;
                    }
                    parsed.push_back(std::move(icon));

                    SkipWhitespace(cursor);
                    if (Consume(cursor, ','))
                    {
                        continue;
                    }
                }
            }
            else
            {
                if (!SkipValue(cursor))
                {
                    outError = "Vector icon JSON contains an unreadable value.";
                    return false;
                }
            }

            SkipWhitespace(cursor);
            if (Consume(cursor, ','))
            {
                continue;
            }
        }

        if (parsed.empty())
        {
            outError = "Vector icon JSON loaded zero icons.";
            return false;
        }

        m_icons = std::move(parsed);
        return true;
    }

    const VectorIcon* VectorIconLibrary::FindById(const std::string& id) const
    {
        for (const VectorIcon& icon : m_icons)
        {
            if (icon.id == id)
            {
                return &icon;
            }
        }
        return nullptr;
    }

    const std::vector<VectorIcon>& VectorIconLibrary::GetIcons() const
    {
        return m_icons;
    }
}

