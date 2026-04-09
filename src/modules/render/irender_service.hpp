#pragma once

#include "../../prototypes/presentation/frame.hpp"
#include "render_types.hpp"

#include <cstddef>
#include <cstdint>

class IRenderService
{
public:
    virtual ~IRenderService() = default;

    virtual void SubmitFrame(const FramePrototype& frame) = 0;
    virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
    virtual bool ResizeToClientArea() = 0;
    virtual void Clear(std::uint32_t color) = 0;
    virtual void PutPixel(int x, int y, std::uint32_t color) = 0;
    virtual void DrawLine(int x0, int y0, int x1, int y1, std::uint32_t color) = 0;
    virtual void DrawRect(int x, int y, int width, int height, std::uint32_t color) = 0;
    virtual void FillRect(int x, int y, int width, int height, std::uint32_t color) = 0;
    virtual void DrawGrid(int cellSize, std::uint32_t color) = 0;
    virtual void DrawCrosshair(int x, int y, int size, std::uint32_t color) = 0;
    virtual void Present() = 0;
    virtual int GetBackbufferWidth() const = 0;
    virtual int GetBackbufferHeight() const = 0;
    virtual double GetAspectRatio() const = 0;
    virtual std::size_t GetSubmittedItemCount() const = 0;
    virtual std::size_t GetLegacyCompatibilityCommandCount() const = 0;
    virtual bool IsStarted() const = 0;
};
