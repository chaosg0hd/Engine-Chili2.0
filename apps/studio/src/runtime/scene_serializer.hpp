#pragma once

#include "runtime/runtime_world.hpp"

#include <string>

namespace studio_runtime
{
    class SceneSerializer
    {
    public:
        static bool LoadFromFile(const std::string& path, RuntimeWorld& world, std::string& outError);
        static bool SaveToFile(const std::string& path, const RuntimeWorld& world, std::string& outError);
        static bool LoadFromText(const std::string& text, RuntimeWorld& world, std::string& outError);
        static std::string SaveToText(const RuntimeWorld& world);
    };
}
