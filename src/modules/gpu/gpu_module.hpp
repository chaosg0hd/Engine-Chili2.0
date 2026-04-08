#pragma once

#include "../../core/module.hpp"
#include "igpu_service.hpp"
#include "../render/render_frame_context.hpp"
#include "../render/render_types.hpp"
#include "../render/scene/render_scene.hpp"

#include <cstdint>
#include <memory>

class EngineContext;
class PlatformModule;
class IRenderBackend;

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

    void RenderFrame(const RenderScene& scene, const RenderClearColor& clearColor, float deltaTime);
    void Resize(std::uint32_t width, std::uint32_t height);
    bool ResizeToSurface();

    int GetBackbufferWidth() const;
    int GetBackbufferHeight() const;
    double GetAspectRatio() const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    bool CreateBackend(EngineContext& context);
    void DestroyBackend();
    void UpdateViewportFromPlatform();

private:
    bool m_initialized = false;
    bool m_started = false;
    std::uint64_t m_frameIndex = 0;

    PlatformModule* m_platform = nullptr;
    RenderBackendType m_backendType = RenderBackendType::DirectX11;
    std::unique_ptr<IRenderBackend> m_backend;
    RenderViewport m_viewport;
};
