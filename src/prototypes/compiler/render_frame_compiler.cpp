#include "render_frame_compiler.hpp"

#include "infinite_plane_render_compiler.hpp"
#include "object_render_compiler.hpp"

#include "../presentation/item.hpp"
#include "../presentation/pass.hpp"
#include "../presentation/view.hpp"

namespace
{
    RenderPassDataKind CompilePassKind(PassKind kind)
    {
        switch (kind)
        {
        case PassKind::Scene:
            return RenderPassDataKind::Scene;
        case PassKind::Overlay:
            return RenderPassDataKind::Overlay;
        case PassKind::Composite:
            return RenderPassDataKind::Composite;
        default:
            return RenderPassDataKind::Unknown;
        }
    }

    RenderViewDataKind CompileViewKind(ViewKind kind)
    {
        switch (kind)
        {
        case ViewKind::Scene3D:
            return RenderViewDataKind::Scene3D;
        case ViewKind::Overlay2D:
            return RenderViewDataKind::Overlay2D;
        default:
            return RenderViewDataKind::Unknown;
        }
    }

    RenderCameraData CompileCamera(const CameraPrototype& camera)
    {
        RenderCameraData cameraData;
        cameraData.position = RenderVector3(camera.position.x, camera.position.y, camera.position.z);
        cameraData.target = RenderVector3(camera.target.x, camera.target.y, camera.target.z);
        cameraData.up = RenderVector3(camera.up.x, camera.up.y, camera.up.z);
        cameraData.fovDegrees = camera.fovDegrees;
        cameraData.nearPlane = camera.nearPlane;
        cameraData.farPlane = camera.farPlane;
        cameraData.exposure = camera.exposure;
        return cameraData;
    }

    void CompileLights(
        const std::vector<SceneLightPrototype>& sourceLights,
        std::vector<RenderSceneLightData>& outLights)
    {
        outLights.reserve(sourceLights.size());
        for (const SceneLightPrototype& light : sourceLights)
        {
            RenderSceneLightData lightData;
            lightData.position = RenderVector3(light.position.x, light.position.y, light.position.z);
            lightData.color = light.color.ToArgb();
            lightData.intensity = light.intensity;
            lightData.range = light.range;
            lightData.enabled = light.enabled;
            outLights.push_back(lightData);
        }
    }

    void CompileLightRayEmitters(
        const std::vector<LightRayEmitterPrototype>& sourceEmitters,
        std::vector<RenderLightRayData>& outEmitters)
    {
        outEmitters.reserve(sourceEmitters.size());
        for (const LightRayEmitterPrototype& emitter : sourceEmitters)
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
            outEmitters.push_back(lightData);
        }
    }

    RenderItemData CompileOverlayItem()
    {
        RenderItemData itemData;
        itemData.kind = RenderItemDataKind::Overlay2D;
        return itemData;
    }

    RenderItemData CompileScreenPatchItem(const ItemPrototype& item)
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
        return itemData;
    }

    void CompileItems(
        const ViewPrototype& sourceView,
        std::vector<RenderItemData>& outItems)
    {
        outItems.reserve(sourceView.items.size());
        for (const ItemPrototype& item : sourceView.items)
        {
            switch (item.kind)
            {
            case ItemKind::Object3D:
                ObjectRenderCompiler::Append(item.object3D, outItems);
                break;
            case ItemKind::InfinitePlane:
                InfinitePlaneRenderCompiler::Append(item.infinitePlane, sourceView.camera, outItems);
                break;
            case ItemKind::Overlay2D:
                outItems.push_back(CompileOverlayItem());
                break;
            case ItemKind::ScreenPatch:
            case ItemKind::ScreenHexPatch:
                outItems.push_back(CompileScreenPatchItem(item));
                break;
            default:
                break;
            }
        }
    }

    RenderViewData CompileView(const ViewPrototype& view)
    {
        RenderViewData viewData;
        viewData.kind = CompileViewKind(view.kind);
        viewData.camera = CompileCamera(view.camera);
        CompileLights(view.lights, viewData.lights);
        CompileLightRayEmitters(view.lightRayEmitters, viewData.lightRayEmitters);
        CompileItems(view, viewData.items);
        return viewData;
    }

    RenderPassData CompilePass(const PassPrototype& pass)
    {
        RenderPassData passData;
        passData.kind = CompilePassKind(pass.kind);
        passData.views.reserve(pass.views.size());
        for (const ViewPrototype& view : pass.views)
        {
            passData.views.push_back(CompileView(view));
        }

        return passData;
    }
}

RenderFrameData RenderFrameCompiler::Compile(const FramePrototype& frame)
{
    RenderFrameData result;
    result.passes.reserve(frame.passes.size());

    for (const PassPrototype& pass : frame.passes)
    {
        result.passes.push_back(CompilePass(pass));
    }

    return result;
}
