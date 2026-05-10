#include "runtime/runtime_registry.hpp"

#include "runtime/studio_preview_runtime.hpp"
#if defined(ENGINE_STUDIO_ENABLE_SAMPLE_RUNTIMES)
#include "runtime/hello_game_runtime.hpp"
#endif

namespace studio_runtime
{
    RuntimeRegistry::RuntimeRegistry()
    {
#if defined(ENGINE_STUDIO_ENABLE_SAMPLE_RUNTIMES)
        Register(
            "HelloGameRuntime",
            []
            {
                return std::make_unique<HelloGameRuntime>();
            });
#endif
        Register(
            "StudioPreviewRuntime",
            []
            {
                return std::make_unique<StudioPreviewRuntime>();
            });
        // TEMPORARY EXCEPTION (per ARCHI_RULES):
        //   RuntimeRegistry is an in-process shortcut for editor preview only.
        //   Long-term owner: project build system via exported artifact DLLs.
        //   Deferral reason: launcher→engine.dll→app DLL stack not yet wired up.
        //   Only Studio-owned preview stubs belong here.
        //   Project-specific runtime classes must NOT be registered here — they must
        //   come from project-built artifacts loaded at runtime.
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
