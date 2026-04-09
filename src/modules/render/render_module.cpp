#include "render_module.hpp"

#include "../../core/engine_context.hpp"
#include "../../prototypes/entity/camera.hpp"
#include "../../prototypes/entity/material.hpp"
#include "../../prototypes/entity/mesh.hpp"
#include "../../prototypes/entity/object.hpp"
#include "../../prototypes/math/math.hpp"
#include "../../prototypes/presentation/item.hpp"
#include "../../prototypes/presentation/pass.hpp"
#include "../../prototypes/presentation/view.hpp"
#include "../gpu/igpu_service.hpp"
#include "../logger/logger_module.hpp"

RenderModule::RenderModule() = default;
RenderModule::~RenderModule() = default;

const char* RenderModule::GetName() const
{
    return "Render";
}

bool RenderModule::Initialize(EngineContext& context)
{
    if (m_initialized)
    {
        return true;
    }

    if (!m_gpu)
    {
        m_gpu = context.Gpu;
    }

    m_initialized = true;

    if (context.Logger)
    {
        context.Logger->Info("RenderModule initialized.");
    }

    return true;
}

void RenderModule::Startup(EngineContext& context)
{
    if (!m_initialized || m_started)
    {
        return;
    }

    if (!m_gpu)
    {
        m_gpu = context.Gpu;
    }

    m_started = true;

    if (context.Logger)
    {
        context.Logger->Info("RenderModule started.");
    }
}

void RenderModule::Update(EngineContext& context, float deltaTime)
{
    if (!m_gpu)
    {
        m_gpu = context.Gpu;
    }

    if (!m_initialized || !m_started || !m_gpu)
    {
        return;
    }

    m_gpu->RenderFrame(m_frame, m_clearColor, deltaTime);
}

void RenderModule::Shutdown(EngineContext& context)
{
    if (!m_initialized)
    {
        return;
    }

    m_frame = RenderFrameData{};
    m_clearColor = RenderClearColor{};
    m_legacyCompatibilityCommandCount = 0;
    m_gpu = nullptr;

    m_started = false;
    m_initialized = false;

    if (context.Logger)
    {
        context.Logger->Info("RenderModule shutdown.");
    }
}

void RenderModule::SubmitFrame(const FramePrototype& frame)
{
    m_frame = BuildRenderFrameData(frame);
}

void RenderModule::Resize(std::uint32_t width, std::uint32_t height)
{
    if (m_gpu)
    {
        m_gpu->Resize(width, height);
    }
}

bool RenderModule::ResizeToClientArea()
{
    return m_gpu ? m_gpu->ResizeToSurface() : false;
}

RenderClearColor RenderModule::ToClearColor(std::uint32_t color)
{
    RenderClearColor clearColor;
    clearColor.a = static_cast<float>((color >> 24) & 0xFFu) / 255.0f;
    clearColor.r = static_cast<float>((color >> 16) & 0xFFu) / 255.0f;
    clearColor.g = static_cast<float>((color >> 8) & 0xFFu) / 255.0f;
    clearColor.b = static_cast<float>(color & 0xFFu) / 255.0f;
    return clearColor;
}

void RenderModule::Clear(std::uint32_t color)
{
    m_clearColor = ToClearColor(color);
}

void RenderModule::PutPixel(int x, int y, std::uint32_t color)
{
    (void)x;
    (void)y;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::DrawLine(int x0, int y0, int x1, int y1, std::uint32_t color)
{
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::DrawRect(int x, int y, int width, int height, std::uint32_t color)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::FillRect(int x, int y, int width, int height, std::uint32_t color)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::DrawGrid(int cellSize, std::uint32_t color)
{
    (void)cellSize;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::DrawCrosshair(int x, int y, int size, std::uint32_t color)
{
    (void)x;
    (void)y;
    (void)size;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::Present()
{
    // Presentation belongs to the GPU-owned frame flow now.
    // Keep this as a no-op only for older engine-facing compatibility calls.
}

int RenderModule::GetBackbufferWidth() const
{
    return m_gpu ? m_gpu->GetBackbufferWidth() : 0;
}

int RenderModule::GetBackbufferHeight() const
{
    return m_gpu ? m_gpu->GetBackbufferHeight() : 0;
}

double RenderModule::GetAspectRatio() const
{
    return m_gpu ? m_gpu->GetAspectRatio() : 0.0;
}

std::size_t RenderModule::GetSubmittedItemCount() const
{
    return CountFrameItems(m_frame);
}

std::size_t RenderModule::GetLegacyCompatibilityCommandCount() const
{
    return m_legacyCompatibilityCommandCount;
}

bool RenderModule::IsInitialized() const
{
    return m_initialized;
}

bool RenderModule::IsStarted() const
{
    return m_started;
}

RenderFrameData RenderModule::BuildRenderFrameData(const FramePrototype& frame)
{
    RenderFrameData result;
    result.passes.reserve(frame.passes.size());

    for (const PassPrototype& pass : frame.passes)
    {
        RenderPassData passData;
        switch (pass.kind)
        {
        case PassKind::Scene:
            passData.kind = RenderPassDataKind::Scene;
            break;
        case PassKind::Overlay:
            passData.kind = RenderPassDataKind::Overlay;
            break;
        case PassKind::Composite:
            passData.kind = RenderPassDataKind::Composite;
            break;
        default:
            passData.kind = RenderPassDataKind::Unknown;
            break;
        }

        passData.views.reserve(pass.views.size());
        for (const ViewPrototype& view : pass.views)
        {
            RenderViewData viewData;
            switch (view.kind)
            {
            case ViewKind::Scene3D:
                viewData.kind = RenderViewDataKind::Scene3D;
                break;
            case ViewKind::Overlay2D:
                viewData.kind = RenderViewDataKind::Overlay2D;
                break;
            default:
                viewData.kind = RenderViewDataKind::Unknown;
                break;
            }

            viewData.camera.position = RenderVector3(view.camera.position.x, view.camera.position.y, view.camera.position.z);
            viewData.camera.target = RenderVector3(view.camera.target.x, view.camera.target.y, view.camera.target.z);
            viewData.camera.up = RenderVector3(view.camera.up.x, view.camera.up.y, view.camera.up.z);
            viewData.camera.fovDegrees = view.camera.fovDegrees;
            viewData.camera.nearPlane = view.camera.nearPlane;
            viewData.camera.farPlane = view.camera.farPlane;

            viewData.items.reserve(view.items.size());
            for (const ItemPrototype& item : view.items)
            {
                RenderItemData itemData;
                switch (item.kind)
                {
                case ItemKind::Object3D:
                    itemData.kind = RenderItemDataKind::Object3D;
                    break;
                case ItemKind::Overlay2D:
                    itemData.kind = RenderItemDataKind::Overlay2D;
                    break;
                default:
                    itemData.kind = RenderItemDataKind::Unknown;
                    break;
                }

                itemData.object3D.transform.translation = RenderVector3(
                    item.object3D.transform.translation.x,
                    item.object3D.transform.translation.y,
                    item.object3D.transform.translation.z);
                itemData.object3D.transform.rotationRadians = RenderVector3(
                    item.object3D.transform.rotationRadians.x,
                    item.object3D.transform.rotationRadians.y,
                    item.object3D.transform.rotationRadians.z);
                itemData.object3D.transform.scale = RenderVector3(
                    item.object3D.transform.scale.x,
                    item.object3D.transform.scale.y,
                    item.object3D.transform.scale.z);

                itemData.object3D.mesh.handle = item.object3D.mesh.handle;
                switch (item.object3D.mesh.builtInKind)
                {
                case BuiltInMeshKind::Triangle:
                    itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Triangle;
                    break;
                case BuiltInMeshKind::Diamond:
                    itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Diamond;
                    break;
                case BuiltInMeshKind::Cube:
                    itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Cube;
                    break;
                case BuiltInMeshKind::Quad:
                    itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Quad;
                    break;
                case BuiltInMeshKind::Octahedron:
                    itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::Octahedron;
                    break;
                default:
                    itemData.object3D.mesh.builtInKind = RenderBuiltInMeshKind::None;
                    break;
                }

                itemData.object3D.material.handle = item.object3D.material.handle;
                itemData.object3D.material.baseColor = item.object3D.material.baseColor;
                viewData.items.push_back(std::move(itemData));
            }

            passData.views.push_back(std::move(viewData));
        }

        result.passes.push_back(std::move(passData));
    }

    return result;
}

std::size_t RenderModule::CountFrameItems(const RenderFrameData& frame)
{
    std::size_t itemCount = 0;

    for (const RenderPassData& pass : frame.passes)
    {
        for (const RenderViewData& view : pass.views)
        {
            itemCount += view.items.size();
        }
    }

    return itemCount;
}
