#include "runtime/runtime_registry.hpp"

#include "runtime/hello_game_runtime.hpp"
#include "runtime/studio_preview_runtime.hpp"

namespace studio_runtime
{
    RuntimeRegistry::RuntimeRegistry()
    {
        Register(
            "HelloGameRuntime",
            []
            {
                return std::make_unique<HelloGameRuntime>();
            });
        Register(
            "StudioPreviewRuntime",
            []
            {
                return std::make_unique<StudioPreviewRuntime>();
            });
        // Project runtimes must be loaded from project-owned build artifacts.
        // Studio must not register game-specific runtime classes here.
    }

    void RuntimeRegistry::Register(const std::string& runtimeName, RuntimeFactory factory)
    {
        if (runtimeName.empty() || !factory)
        {
            return;
        }

        m_factories[runtimeName] = std::move(factory);
    }

    std::unique_ptr<IGameRuntime> RuntimeRegistry::Create(const std::string& runtimeName) const
    {
        const auto found = m_factories.find(runtimeName);
        if (found == m_factories.end())
        {
            return nullptr;
        }

        return found->second();
    }

    bool RuntimeRegistry::Contains(const std::string& runtimeName) const
    {
        return m_factories.find(runtimeName) != m_factories.end();
    }
}
