#pragma once

#include "module.hpp"

#include <vector>
#include <memory>

class IModule;
class EngineContext;

class ModuleManager
{
public:
    template<typename T>
    T* AddModule()
    {
        auto module = std::make_unique<T>();
        T* ptr = module.get();
        m_modules.push_back(std::move(module));
        return ptr;
    }

    bool InitializeAll(EngineContext& context);
    void StartupAll(EngineContext& context);
    void UpdateAll(EngineContext& context, float deltaTime);
    void ShutdownAll(EngineContext& context);
    void Clear();

private:
    std::vector<std::unique_ptr<IModule>> m_modules;
};
