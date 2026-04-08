#include "render_backend_factory.hpp"

#include "backend/dx11_render_backend.hpp"
#include "backend/irender_backend.hpp"
#include "backend/null_render_backend.hpp"

#include <memory>

std::unique_ptr<IRenderBackend> RenderBackendFactory::Create(RenderBackendType type)
{
    switch (type)
    {
    case RenderBackendType::DirectX11:
        return std::make_unique<Dx11RenderBackend>();

    case RenderBackendType::Null:
    default:
        return std::make_unique<NullRenderBackend>();
    }
}
