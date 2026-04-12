#include "module_manager.hpp"
#include "module.hpp"
#include "engine_context.hpp"

bool ModuleManager::InitializeAll(EngineContext& context)
{
    for (auto& m : m_modules)
    {
        if (!m->Initialize(context))
            return false;
    }

    return true;
}

void ModuleManager::StartupAll(EngineContext& context)
{
    for (auto& m : m_modules)
        m->Startup(context);
}

void ModuleManager::UpdateAll(EngineContext& context, float deltaTime)
{
    for (auto& m : m_modules)
        m->Update(context, deltaTime);
}

void ModuleManager::ShutdownAll(EngineContext& context)
{
    for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it)
        (*it)->Shutdown(context);
}

void ModuleManager::Clear()
{
    m_modules.clear();
}
