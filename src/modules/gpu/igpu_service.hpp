#pragma once

#include "../render/render_types.hpp"
#include "../render/render_frame_data.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

enum class GpuResourceKind : std::uint8_t
{
    Unknown = 0,
    Texture,
    MeshData,
    MaterialData,
    ShaderBlob
};

using GpuResourceHandle = std::uint32_t;

struct GpuUploadRequest
{
    GpuResourceKind kind = GpuResourceKind::Unknown;
    const void* data = nullptr;
    std::size_t size = 0;
    std::string debugName;
};

class IGpuService
{
public:
    virtual ~IGpuService() = default;

    virtual void SetBackendType(RenderBackendType type) = 0;
    virtual RenderBackendType GetBackendType() const = 0;
    virtual const char* GetBackendName() const = 0;

    virtual void RenderFrame(const RenderFrameData& frame, const RenderClearColor& clearColor, float deltaTime) = 0;
    virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
    virtual bool ResizeToSurface() = 0;

    virtual int GetBackbufferWidth() const = 0;
    virtual int GetBackbufferHeight() const = 0;
    virtual double GetAspectRatio() const = 0;

    virtual GpuResourceHandle CreateUploadedResource(const GpuUploadRequest& request) = 0;
    virtual bool DestroyResource(GpuResourceHandle handle) = 0;
    virtual bool IsResourceValid(GpuResourceHandle handle) const = 0;
    virtual GpuResourceKind GetResourceKind(GpuResourceHandle handle) const = 0;
    virtual std::size_t GetResourceSize(GpuResourceHandle handle) const = 0;
    virtual std::size_t GetResourceCount() const = 0;
    virtual std::size_t GetTotalResourceBytes() const = 0;
};
