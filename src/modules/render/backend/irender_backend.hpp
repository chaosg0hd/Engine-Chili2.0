#pragma once

#include "../render_frame_data.hpp"
#include "../render_frame_context.hpp"
#include "../render_types.hpp"
#include "../../gpu/igpu_service.hpp"

#include <cstdint>

class EngineContext;
struct GpuTaskDesc;

class IRenderBackend
{
public:
    virtual ~IRenderBackend() = default;

    virtual const char* GetName() const = 0;

    virtual bool Initialize(
        EngineContext& context,
        void* nativeWindowHandle,
        std::uint32_t width,
        std::uint32_t height) = 0;
    virtual void Shutdown() = 0;

    virtual void BeginFrame(const RenderFrameContext& frameContext) = 0;
    virtual void Render(const RenderFrameData& frame) = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

    virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
    virtual bool CreateResource(GpuResourceHandle handle, const GpuUploadRequest& request) = 0;
    virtual void DestroyResource(GpuResourceHandle handle) = 0;

    virtual bool SupportsComputeDispatch() const { return false; }
    virtual bool SubmitGpuTask(const GpuTaskDesc& task) { (void)task; return false; }
    virtual void WaitForGpuIdle() {}
};
