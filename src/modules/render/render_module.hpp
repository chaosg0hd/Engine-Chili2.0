#pragma once

#include "../../core/module.hpp"
#include "render_types.hpp"
#include "scene/render_scene.hpp"

#include <cstdint>

class EngineContext;
class IGpuService;

class RenderModule : public IModule
{
public:
    RenderModule();
    ~RenderModule() override;

    const char* GetName() const override;
    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    void SetBackendType(RenderBackendType type);
    RenderBackendType GetBackendType() const;
    void SubmitScene(const RenderScene& scene);
    void Resize(std::uint32_t width, std::uint32_t height);

    bool ResizeToClientArea();
    void Clear(std::uint32_t color);
    void PutPixel(int x, int y, std::uint32_t color);
    void DrawLine(int x0, int y0, int x1, int y1, std::uint32_t color);
    void DrawRect(int x, int y, int width, int height, std::uint32_t color);
    void FillRect(int x, int y, int width, int height, std::uint32_t color);
    void DrawGrid(int cellSize, std::uint32_t color);
    void DrawCrosshair(int x, int y, int size, std::uint32_t color);
    void Present();

    int GetBackbufferWidth() const;
    int GetBackbufferHeight() const;
    double GetAspectRatio() const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    static RenderClearColor ToClearColor(std::uint32_t color);

private:
    bool m_initialized = false;
    bool m_started = false;

    IGpuService* m_gpu = nullptr;
    RenderScene m_scene;
    RenderClearColor m_clearColor;
};
