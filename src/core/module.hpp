#pragma once

class EngineContext;

class IModule
{
public:
    virtual ~IModule() = default;

    virtual const char* GetName() const = 0;

    virtual bool Initialize(EngineContext& context) = 0;

    virtual void Startup([[maybe_unused]] EngineContext& context) {}
    
    virtual void Update([[maybe_unused]] EngineContext& context,
                        [[maybe_unused]] float deltaTime) {}

    virtual void Shutdown(EngineContext& context) = 0;
};