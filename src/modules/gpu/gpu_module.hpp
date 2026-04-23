#pragma once

#include "../../core/module.hpp"
#include "igpu_service.hpp"
#include "../render/render_frame_data.hpp"
#include "../render/render_frame_context.hpp"
#include "../render/render_types.hpp"

#include <cstdint>
#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>

class EngineContext;
class IPlatformService;
class IRenderBackend;
struct GpuTaskDesc;

class GpuModule : public IModule, public IGpuService
{
public:
    GpuModule();
    ~GpuModule() override;

    const char* GetName() const override;
    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    void SetBackendType(RenderBackendType type);
    RenderBackendType GetBackendType() const;
    const char* GetBackendName() const;

    void RenderFrame(const RenderFrameData& frame, const RenderClearColor& clearColor, float deltaTime);
    void Resize(std::uint32_t width, std::uint32_t height);
    bool ResizeToSurface();

    int GetBackbufferWidth() const;
    int GetBackbufferHeight() const;
    double GetAspectRatio() const;
    void SetDerivedBounceFillSettings(const DerivedBounceFillSettings& settings);
    DerivedBounceFillSettings GetDerivedBounceFillSettings() const;
    void SetTracedIndirectSettings(const TracedIndirectSettings& settings);
    TracedIndirectSettings GetTracedIndirectSettings() const;
    GpuResourceHandle CreateUploadedResource(const GpuUploadRequest& request);
    bool DestroyResource(GpuResourceHandle handle);
    bool IsResourceValid(GpuResourceHandle handle) const;
    GpuResourceKind GetResourceKind(GpuResourceHandle handle) const;
    std::size_t GetResourceSize(GpuResourceHandle handle) const;
    std::size_t GetResourceCount() const;
    std::size_t GetTotalResourceBytes() const;
    bool SupportsGpuBuffers() const;
    bool SupportsComputeDispatch() const;
    bool SubmitGpuTask(const GpuTaskDesc& task);
    void WaitForGpuIdle();

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    bool CreateBackend(EngineContext& context);
    void DestroyBackend();
    void UpdateViewportFromPlatform();

private:
    struct GpuResourceRecord
    {
        GpuResourceHandle handle = 0;
        GpuResourceKind kind = GpuResourceKind::Unknown;
        std::size_t size = 0;
        bool resident = false;
        std::string debugName;
    };

private:
    bool m_initialized = false;
    bool m_started = false;
    std::uint64_t m_frameIndex = 0;
    GpuResourceHandle m_nextResourceHandle = 1U;

    IPlatformService* m_platform = nullptr;
    RenderBackendType m_backendType = RenderBackendType::DirectX11;
    std::unique_ptr<IRenderBackend> m_backend;
    RenderViewport m_viewport;
    mutable std::mutex m_resourceMutex;
    std::unordered_map<GpuResourceHandle, GpuResourceRecord> m_resources;
};
