#pragma once

#include "../../core/module.hpp"
#include "../../prototypes/presentation/frame.hpp"
#include "irender_service.hpp"
#include "render_frame_data.hpp"
#include "render_types.hpp"

#include <cstdint>
#include <cstddef>

class EngineContext;
class IGpuService;

class RenderModule : public IModule, public IRenderService
{
public:
    RenderModule();
    ~RenderModule() override;

    const char* GetName() const override;
    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    void SubmitFrame(const FramePrototype& frame) override;
    void Resize(std::uint32_t width, std::uint32_t height) override;

    bool ResizeToClientArea() override;
    void Clear(std::uint32_t color) override;
    void PutPixel(int x, int y, std::uint32_t color) override;
    void DrawLine(int x0, int y0, int x1, int y1, std::uint32_t color) override;
    void DrawRect(int x, int y, int width, int height, std::uint32_t color) override;
    void FillRect(int x, int y, int width, int height, std::uint32_t color) override;
    void DrawGrid(int cellSize, std::uint32_t color) override;
    void DrawCrosshair(int x, int y, int size, std::uint32_t color) override;
    void Present() override;

    int GetBackbufferWidth() const override;
    int GetBackbufferHeight() const override;
    double GetAspectRatio() const override;
    std::size_t GetSubmittedItemCount() const override;
    std::size_t GetLegacyCompatibilityCommandCount() const override;

    bool IsInitialized() const;
    bool IsStarted() const override;

private:
    static RenderClearColor ToClearColor(std::uint32_t color);
    static RenderFrameData BuildRenderFrameData(const FramePrototype& frame);
    static std::size_t CountFrameItems(const RenderFrameData& frame);

private:
    bool m_initialized = false;
    bool m_started = false;

    IGpuService* m_gpu = nullptr;
    RenderFrameData m_frame;
    RenderClearColor m_clearColor;
    std::size_t m_legacyCompatibilityCommandCount = 0;
};
