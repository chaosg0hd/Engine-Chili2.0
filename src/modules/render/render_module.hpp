#pragma once

#include "../../core/module.hpp"

#include <cstdint>
#include <vector>

class EngineContext;
class PlatformModule;

class RenderModule : public IModule
{
public:
    RenderModule();

    const char* GetName() const override;
    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    void SetPlatformModule(PlatformModule* platform);

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
    bool ResizeBackbuffer(int width, int height);

private:
    bool m_initialized = false;
    bool m_started = false;

    PlatformModule* m_platform = nullptr;

    int m_width = 0;
    int m_height = 0;

    std::vector<std::uint32_t> m_backbuffer;
};
