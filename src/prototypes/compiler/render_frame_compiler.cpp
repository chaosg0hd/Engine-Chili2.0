#include "render_frame_compiler.hpp"

#include "infinite_plane_render_compiler.hpp"
#include "object_render_compiler.hpp"

#include "../presentation/item.hpp"
#include "../presentation/pass.hpp"
#include "../presentation/view.hpp"

RenderFrameData RenderFrameCompiler::Compile(const FramePrototype& frame)
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

            viewData.lights.reserve(view.lightRayEmitters.size());
            for (const LightRayEmitterPrototype& emitter : view.lightRayEmitters)
            {
                RenderLightRayData lightData;
                lightData.origin = RenderVector3(emitter.origin.x, emitter.origin.y, emitter.origin.z);
                const Vector3 direction = emitter.direction;
                lightData.direction = RenderVector3(direction.x, direction.y, direction.z);
                lightData.color = emitter.color.ToArgb();
                lightData.intensity = emitter.intensity;
                lightData.rayCount = emitter.rayCount;
                lightData.maxBounceCount = emitter.maxBounceCount;
                lightData.randomSeed = emitter.randomSeed;
                lightData.spreadAngleRadians = emitter.spreadAngleRadians;
                lightData.maxDistance = emitter.maxDistance;
                lightData.enabled = emitter.enabled && emitter.IsValid();
                viewData.lights.push_back(lightData);
            }

            viewData.items.reserve(view.items.size());
            for (const ItemPrototype& item : view.items)
            {
                switch (item.kind)
                {
                case ItemKind::Object3D:
                    ObjectRenderCompiler::Append(item.object3D, viewData.items);
                    break;
                case ItemKind::InfinitePlane:
                    InfinitePlaneRenderCompiler::Append(item.infinitePlane, view.camera, viewData.items);
                    break;
                case ItemKind::Overlay2D:
                {
                    RenderItemData itemData;
                    itemData.kind = RenderItemDataKind::Overlay2D;
                    viewData.items.push_back(std::move(itemData));
                    break;
                }
                case ItemKind::ScreenPatch:
                case ItemKind::ScreenHexPatch:
                {
                    RenderItemData itemData;
                    itemData.kind = item.kind == ItemKind::ScreenHexPatch
                        ? RenderItemDataKind::ScreenHexPatch
                        : RenderItemDataKind::ScreenPatch;
                    itemData.screenPatch.centerX = item.screenPatch.centerX;
                    itemData.screenPatch.centerY = item.screenPatch.centerY;
                    itemData.screenPatch.halfWidth = item.screenPatch.halfWidth;
                    itemData.screenPatch.halfHeight = item.screenPatch.halfHeight;
                    itemData.screenPatch.rotationRadians = item.screenPatch.rotationRadians;
                    itemData.screenPatch.color = item.screenPatch.color;
                    viewData.items.push_back(std::move(itemData));
                    break;
                }
                default:
                    break;
                }
            }

            passData.views.push_back(std::move(viewData));
        }

        result.passes.push_back(std::move(passData));
    }

    return result;
}
