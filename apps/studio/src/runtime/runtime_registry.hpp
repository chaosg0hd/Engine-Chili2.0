#pragma once

#include "runtime/runtime_game_api.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace studio_runtime
{
    using RuntimeFactory = std::function<std::unique_ptr<IGameRuntime>()>;

    class RuntimeRegistry
    {
    public:
        RuntimeRegistry();

        void Register(const std::string& runtimeName, RuntimeFactory factory);
        std::unique_ptr<IGameRuntime> Create(const std::string& runtimeName) const;
        bool Contains(const std::string& runtimeName) const;

    private:
        std::unordered_map<std::string, RuntimeFactory> m_factories;
    };
}
