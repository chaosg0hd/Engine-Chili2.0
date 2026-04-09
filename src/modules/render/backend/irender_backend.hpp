#pragma once

#include "../../../prototypes/render/render_frame.hpp"
#include "../render_frame_context.hpp"
#include "../render_types.hpp"

#include <cstdint>

class EngineContext;

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
    virtual void Render(const RenderFramePrototype& frame) = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

    virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
};
