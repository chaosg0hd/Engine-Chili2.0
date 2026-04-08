#pragma once

#include "irender_backend.hpp"

class LoggerModule;

class NullRenderBackend : public IRenderBackend
{
public:
    NullRenderBackend();

    const char* GetName() const override;

    bool Initialize(
        EngineContext& context,
        void* nativeWindowHandle,
        std::uint32_t width,
        std::uint32_t height) override;
    void Shutdown() override;

    void BeginFrame(const RenderFrameContext& frameContext) override;
    void Render(const RenderScene& scene) override;
    void EndFrame() override;
    void Present() override;

    void Resize(std::uint32_t width, std::uint32_t height) override;

private:
    bool m_initialized = false;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    LoggerModule* m_logger = nullptr;
};
