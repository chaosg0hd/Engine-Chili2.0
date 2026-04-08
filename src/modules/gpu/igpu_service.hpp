#pragma once

#include "../render/render_types.hpp"
#include "../render/scene/render_scene.hpp"

#include <cstdint>

class IGpuService
{
public:
    virtual ~IGpuService() = default;

    virtual void SetBackendType(RenderBackendType type) = 0;
    virtual RenderBackendType GetBackendType() const = 0;
    virtual const char* GetBackendName() const = 0;

    virtual void RenderFrame(const RenderScene& scene, const RenderClearColor& clearColor, float deltaTime) = 0;
    virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
    virtual bool ResizeToSurface() = 0;

    virtual int GetBackbufferWidth() const = 0;
    virtual int GetBackbufferHeight() const = 0;
    virtual double GetAspectRatio() const = 0;
};
