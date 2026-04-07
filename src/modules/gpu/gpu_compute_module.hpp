#pragma once

#include "../../core/module.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

class EngineContext;

struct GpuBufferView
{
    const std::byte* data = nullptr;
    std::size_t size = 0;
};

struct MutableGpuBufferView
{
    std::byte* data = nullptr;
    std::size_t size = 0;
};

struct GpuTaskDesc
{
    std::string name;
    GpuBufferView input;
    MutableGpuBufferView output;
    std::uint32_t dispatchX = 1;
    std::uint32_t dispatchY = 1;
    std::uint32_t dispatchZ = 1;
    std::uint32_t flags = 0;
};

class GpuComputeModule : public IModule
{
public:
    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    bool IsGpuComputeAvailable() const;
    bool SubmitGpuTask(const GpuTaskDesc& task);
    void WaitForGpuIdle();
    const char* GetBackendName() const;
    bool SupportsGpuBuffers() const;
    bool SupportsComputeDispatch() const;
    std::string GetCapabilitySummary() const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    bool ValidateTask(const GpuTaskDesc& task) const;

private:
    bool m_initialized = false;
    bool m_started = false;
    bool m_gpuComputeAvailable = false;
    std::string m_backendName = "Stub";
    std::string m_lastTaskName;
};
