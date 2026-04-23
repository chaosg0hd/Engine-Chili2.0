#include "render_frame_compiler.hpp"

#include "infinite_plane_render_compiler.hpp"
#include "line_render_compiler.hpp"
#include "object_render_compiler.hpp"

#include "../presentation/item.hpp"
#include "../presentation/pass.hpp"
#include "../presentation/view.hpp"

#include <algorithm>
#include <array>

namespace
{
    constexpr std::size_t kOmniShadowFaceCount = 6U;

    RenderPassDataKind CompilePassKind(PassKind kind)
    {
        switch (kind)
        {
        case PassKind::Shadow:
            return RenderPassDataKind::Shadow;
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
        case ViewKind::ShadowCubemapFace:
            return RenderViewDataKind::ShadowCubemapFace;
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
        const Vector3 worldPosition = camera.GetWorldPosition();
        const Vector3 target = camera.GetTargetPosition();
        const Vector3 forward = camera.GetForward();
        const Vector3 right = camera.GetRight();
        const Vector3 up = camera.GetUpDirection();
        cameraData.worldPosition = RenderVector3(worldPosition.x, worldPosition.y, worldPosition.z);
        cameraData.target = RenderVector3(target.x, target.y, target.z);
        cameraData.forward = RenderVector3(forward.x, forward.y, forward.z);
        cameraData.right = RenderVector3(right.x, right.y, right.z);
        cameraData.up = RenderVector3(up.x, up.y, up.z);
        cameraData.projectionMode = camera.projection.mode == CameraProjectionModePrototype::Orthographic
            ? RenderProjectionMode::Orthographic
            : RenderProjectionMode::Perspective;
        cameraData.fieldOfViewDegrees = camera.projection.fieldOfViewDegrees;
        cameraData.orthographicHeight = camera.projection.orthographicHeight;
        cameraData.nearPlane = camera.projection.nearPlane;
        cameraData.farPlane = camera.projection.farPlane;
        cameraData.aspectRatioOverride = camera.projection.aspectRatioOverride;
        cameraData.exposure = camera.GetExposureScalar();
        return cameraData;
    }

    RenderShadowPolicyData CompileDirectLightVisibilityPolicy(const LightVisibilityPolicyPrototype& policy)
    {
        RenderShadowPolicyData shadowData;
        shadowData.enabled = policy.enabled;
        shadowData.baseMethod = policy.method == LightVisibilityMethodPrototype::RasterCubemap
            ? RenderShadowBaseMethod::RasterCubemap
            : RenderShadowBaseMethod::None;
        shadowData.refinementMethod = RenderShadowRefinementMethod::None;
        shadowData.updatePolicy = RenderShadowUpdatePolicy::Continuous;
        shadowData.resolution = policy.resolution;
        shadowData.depthBias = policy.depthBias;
        shadowData.normalBias = policy.normalBias;
        shadowData.nearPlane = policy.nearPlane;
        shadowData.farPlane = policy.farPlane;
        shadowData.refinementInfluence = policy.refinementInfluence;
        return shadowData;
    }

    CompiledLightType CompileLightType(LightEmitterKind kind)
    {
        switch (kind)
        {
        case LightEmitterKind::Directional:
            return CompiledLightType::Directional;
        case LightEmitterKind::Spot:
            return CompiledLightType::Spot;
        case LightEmitterKind::Point:
        default:
            return CompiledLightType::Point;
        }
    }

    CompiledVisibilityMode CompileVisibilityMode(const LightVisibilityKind kind)
    {
        switch (kind)
        {
        case LightVisibilityKind::RasterShadowCubemap:
            return CompiledVisibilityMode::RasterCubemap;
        case LightVisibilityKind::RasterShadowMap:
            return CompiledVisibilityMode::RasterShadowMap;
        case LightVisibilityKind::Traced:
            return CompiledVisibilityMode::Traced;
        case LightVisibilityKind::None:
        default:
            return CompiledVisibilityMode::None;
        }
    }

    // Transitional bridge: the current render frame still lowers only the active
    // point-light + direct-BRDF path while the new LightPrototype family becomes
    // the source authoring contract.
    bool SupportsCurrentSceneLightPath(const LightPrototype& light)
    {
        return light.enabled &&
               light.emitter.kind == LightEmitterKind::Point &&
               light.transport.kind == LightTransportKind::DirectAnalytic &&
               light.interaction.directBrdf;
    }

    const LightVisibilityPolicyPrototype& GetDirectLightVisibilityPolicy(const LightPrototype& light)
    {
        return light.visibility.policy;
    }

    struct DirectLightVisibilitySource
    {
        std::size_t sourceViewIndex = 0U;
        std::size_t lightIndex = 0U;
        LightVisibilityPolicyPrototype policy;
    };

    bool TryFindDirectLightVisibilitySource(const PassPrototype& pass, DirectLightVisibilitySource& outSource)
    {
        for (std::size_t viewIndex = 0; viewIndex < pass.views.size(); ++viewIndex)
        {
            const ViewPrototype& view = pass.views[viewIndex];
            if (view.kind != ViewKind::Scene3D)
            {
                continue;
            }

            for (std::size_t lightIndex = 0; lightIndex < view.directLights.size(); ++lightIndex)
            {
                const LightPrototype& light = view.directLights[lightIndex];
                if (!SupportsCurrentSceneLightPath(light) ||
                    light.visibility.kind != LightVisibilityKind::RasterShadowCubemap ||
                    !GetDirectLightVisibilityPolicy(light).IsRasterCubemapEnabled())
                {
                    continue;
                }

                outSource.sourceViewIndex = viewIndex;
                outSource.lightIndex = lightIndex;
                outSource.policy = GetDirectLightVisibilityPolicy(light);
                return true;
            }
        }

        return false;
    }

    void CompileCompiledLights(
        const std::vector<LightPrototype>& sourceLights,
        std::vector<CompiledLightData>& outLights,
        std::size_t currentViewIndex,
        std::size_t visibilitySourceViewIndex,
        std::size_t visibilityLightIndex,
        std::uint32_t activeVisibilityIndex)
    {
        outLights.reserve(sourceLights.size());
        for (std::size_t lightIndex = 0; lightIndex < sourceLights.size(); ++lightIndex)
        {
            const LightPrototype& light = sourceLights[lightIndex];
            CompiledLightData lightData;
            lightData.type = CompileLightType(light.emitter.kind);
            lightData.enabled = light.enabled;
            lightData.affectsDirectLighting = SupportsCurrentSceneLightPath(light);
            lightData.source.position = RenderVector3(
                light.emitter.position.x,
                light.emitter.position.y,
                light.emitter.position.z);
            lightData.source.color = light.emitter.color.ToArgb();
            lightData.source.intensity = light.emitter.intensity;
            lightData.source.range = light.emitter.range;
            lightData.visibilityMode = CompileVisibilityMode(light.visibility.kind);
            lightData.visibilityResourceSlot =
                (activeVisibilityIndex != kInvalidRenderShadowIndex &&
                 currentViewIndex == visibilitySourceViewIndex &&
                 lightIndex == visibilityLightIndex)
                    ? activeVisibilityIndex
                    : kInvalidRenderShadowIndex;
            lightData.visibility.policy = CompileDirectLightVisibilityPolicy(GetDirectLightVisibilityPolicy(light));
            lightData.visibility.activeShadowIndex = lightData.visibilityResourceSlot;
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

    void CompileIndirectLightProbes(
        const std::vector<IndirectLightProbePrototype>& sourceProbes,
        std::vector<RenderIndirectLightProbeData>& outProbes)
    {
        outProbes.reserve(sourceProbes.size());
        for (const IndirectLightProbePrototype& probe : sourceProbes)
        {
            RenderIndirectLightProbeData probeData;
            probeData.position = RenderVector3(probe.position.x, probe.position.y, probe.position.z);
            probeData.indirectColor = probe.indirectColor.ToArgb();
            probeData.intensity = probe.intensity;
            probeData.radius = probe.radius;
            probeData.enabled = probe.enabled;
            outProbes.push_back(probeData);
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
        std::vector<RenderItemData>& outItems,
        bool shadowCastersOnly)
    {
        outItems.reserve(sourceView.items.size());
        for (const ItemPrototype& item : sourceView.items)
        {
            const std::size_t itemStart = outItems.size();
            switch (item.kind)
            {
            case ItemKind::Object3D:
                ObjectRenderCompiler::Append(item.object3D, outItems);
                break;
            case ItemKind::Line:
                LineRenderCompiler::Append(
                    item.line.geometry,
                    item.line.color,
                    item.line.thickness,
                    item.line.fallbackLength,
                    outItems);
                break;
            case ItemKind::InfinitePlane:
                InfinitePlaneRenderCompiler::Append(item.infinitePlane, sourceView.camera, outItems);
                break;
            case ItemKind::Overlay2D:
                if (!shadowCastersOnly)
                {
                    outItems.push_back(CompileOverlayItem());
                }
                break;
            case ItemKind::ScreenPatch:
            case ItemKind::ScreenHexPatch:
                if (!shadowCastersOnly)
                {
                    outItems.push_back(CompileScreenPatchItem(item));
                }
                break;
            default:
                break;
            }

            if (shadowCastersOnly && outItems.size() > itemStart)
            {
                outItems.erase(
                    std::remove_if(
                        outItems.begin() + static_cast<std::ptrdiff_t>(itemStart),
                        outItems.end(),
                        [](const RenderItemData& renderItem)
                        {
                            return renderItem.kind != RenderItemDataKind::Object3D ||
                                   !renderItem.object3D.shadowParticipation.casts;
                        }),
                    outItems.end());
            }
        }
    }

    RenderViewData CompileView(
        const ViewPrototype& view,
        std::size_t currentViewIndex,
        std::size_t visibilitySourceViewIndex = static_cast<std::size_t>(kInvalidRenderShadowIndex),
        std::size_t visibilityLightIndex = static_cast<std::size_t>(kInvalidRenderShadowIndex),
        std::uint32_t activeVisibilityIndex = kInvalidRenderShadowIndex)
    {
        RenderViewData viewData;
        viewData.kind = CompileViewKind(view.kind);
        viewData.camera = CompileCamera(view.camera);
        CompileCompiledLights(
            view.directLights,
            viewData.compiledLights,
            currentViewIndex,
            visibilitySourceViewIndex,
            visibilityLightIndex,
            activeVisibilityIndex);
        CompileIndirectLightProbes(view.indirectLightProbes, viewData.indirectLightProbes);
        CompileLightRayEmitters(view.lightRayEmitters, viewData.lightRayEmitters);
        CompileItems(view, viewData.items, false);
        return viewData;
    }

    RenderViewData BuildShadowView(
        const ViewPrototype& sourceView,
        const LightPrototype& light,
        const LightVisibilityPolicyPrototype& policy,
        std::uint32_t compiledLightWorkIndex,
        std::uint32_t cubemapFaceIndex)
    {
        static const std::array<RenderVector3, kOmniShadowFaceCount> kFaceDirections =
        {
            RenderVector3(1.0f, 0.0f, 0.0f),
            RenderVector3(-1.0f, 0.0f, 0.0f),
            RenderVector3(0.0f, 1.0f, 0.0f),
            RenderVector3(0.0f, -1.0f, 0.0f),
            RenderVector3(0.0f, 0.0f, 1.0f),
            RenderVector3(0.0f, 0.0f, -1.0f)
        };

        static const std::array<RenderVector3, kOmniShadowFaceCount> kFaceUpVectors =
        {
            RenderVector3(0.0f, 1.0f, 0.0f),
            RenderVector3(0.0f, 1.0f, 0.0f),
            RenderVector3(0.0f, 0.0f, -1.0f),
            RenderVector3(0.0f, 0.0f, 1.0f),
            RenderVector3(0.0f, 1.0f, 0.0f),
            RenderVector3(0.0f, 1.0f, 0.0f)
        };

        RenderViewData viewData;
        viewData.kind = RenderViewDataKind::ShadowCubemapFace;
        viewData.compiledLightWorkIndex = compiledLightWorkIndex;
        viewData.directLightVisibilityCubemapFaceIndex = cubemapFaceIndex;
        const RenderVector3 visibilitySourcePosition = RenderVector3(
            light.emitter.position.x,
            light.emitter.position.y,
            light.emitter.position.z);
        viewData.camera.worldPosition = visibilitySourcePosition;
        viewData.camera.target = visibilitySourcePosition + kFaceDirections[cubemapFaceIndex];
        viewData.camera.forward = kFaceDirections[cubemapFaceIndex];
        viewData.camera.right = RenderNormalize(RenderCross(kFaceUpVectors[cubemapFaceIndex], kFaceDirections[cubemapFaceIndex]));
        viewData.camera.up = kFaceUpVectors[cubemapFaceIndex];
        viewData.camera.projectionMode = RenderProjectionMode::Perspective;
        viewData.camera.fieldOfViewDegrees = 90.0f;
        viewData.camera.nearPlane = policy.nearPlane;
        viewData.camera.farPlane = policy.farPlane > policy.nearPlane ? policy.farPlane : light.emitter.range;
        viewData.camera.exposure = 1.0f;
        CompileItems(sourceView, viewData.items, true);
        return viewData;
    }

    CompiledLightWork BuildCompiledLightWork(
        const PassPrototype& pass,
        const DirectLightVisibilitySource& source,
        std::uint32_t visibilityIndex)
    {
        CompiledLightWork work;
        work.compiledLightIndex = static_cast<std::uint32_t>(source.lightIndex);
        work.sourceViewIndex = static_cast<std::uint32_t>(source.sourceViewIndex);
        work.sourceLightIndex = static_cast<std::uint32_t>(source.lightIndex);
        work.requiresVisibilityPass = true;
        work.visibilityPassType = CompiledShadowPassType::RasterCubemap;
        work.visibilityViewCount = static_cast<std::uint32_t>(kOmniShadowFaceCount);
        work.visibilityResourceSlot = visibilityIndex;
        work.visibilityPolicy = CompileDirectLightVisibilityPolicy(source.policy);
        const LightPrototype& light = pass.views[source.sourceViewIndex].directLights[source.lightIndex];
        work.visibilitySourcePosition = RenderVector3(
            light.emitter.position.x,
            light.emitter.position.y,
            light.emitter.position.z);
        work.optionalRefresh = false;
        work.refreshRequiredThisFrame = true;
        return work;
    }

    RenderPassData BuildDirectLightVisibilityPass(
        const PassPrototype& sourcePass,
        const CompiledLightWork& work,
        std::uint32_t compiledLightWorkIndex)
    {
        RenderPassData passData;
        passData.kind = RenderPassDataKind::Shadow;
        passData.compiledLightWorkIndex = compiledLightWorkIndex;
        const ViewPrototype& sourceView = sourcePass.views[work.sourceViewIndex];
        const LightPrototype& light = sourceView.directLights[work.sourceLightIndex];
        passData.views.reserve(kOmniShadowFaceCount);
        for (std::uint32_t faceIndex = 0; faceIndex < static_cast<std::uint32_t>(kOmniShadowFaceCount); ++faceIndex)
        {
            passData.views.push_back(
                BuildShadowView(
                    sourceView,
                    light,
                    LightVisibilityPolicyPrototype{
                        work.visibilityPolicy.enabled,
                        work.visibilityPolicy.baseMethod == RenderShadowBaseMethod::RasterCubemap
                            ? LightVisibilityMethodPrototype::RasterCubemap
                            : LightVisibilityMethodPrototype::None,
                        LightVisibilityRefinementPrototype::None,
                        LightVisibilityUpdatePolicyPrototype::Continuous,
                        work.visibilityPolicy.resolution,
                        work.visibilityPolicy.depthBias,
                        work.visibilityPolicy.normalBias,
                        work.visibilityPolicy.nearPlane,
                        work.visibilityPolicy.farPlane,
                        work.visibilityPolicy.refinementInfluence
                    },
                    compiledLightWorkIndex,
                    faceIndex));
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
        DirectLightVisibilitySource visibilitySource;
        const bool shouldEmitDirectLightVisibilityPass =
            pass.kind == PassKind::Scene && TryFindDirectLightVisibilitySource(pass, visibilitySource);
        const std::uint32_t visibilityIndex =
            shouldEmitDirectLightVisibilityPass ? 0U : kInvalidRenderShadowIndex;

        const std::uint32_t compiledLightWorkIndex =
            shouldEmitDirectLightVisibilityPass
                ? static_cast<std::uint32_t>(result.compiledLightWork.size())
                : kInvalidRenderShadowIndex;
        if (shouldEmitDirectLightVisibilityPass)
        {
            result.compiledLightWork.push_back(BuildCompiledLightWork(pass, visibilitySource, visibilityIndex));
            result.passes.push_back(
                BuildDirectLightVisibilityPass(
                    pass,
                    result.compiledLightWork.back(),
                    compiledLightWorkIndex));
        }

        RenderPassData compiledPass;
        compiledPass.kind = CompilePassKind(pass.kind);
        compiledPass.views.reserve(pass.views.size());
        for (std::size_t viewIndex = 0; viewIndex < pass.views.size(); ++viewIndex)
        {
            const ViewPrototype& view = pass.views[viewIndex];
            compiledPass.views.push_back(
                CompileView(
                    view,
                    viewIndex,
                    shouldEmitDirectLightVisibilityPass
                        ? visibilitySource.sourceViewIndex
                        : static_cast<std::size_t>(kInvalidRenderShadowIndex),
                    shouldEmitDirectLightVisibilityPass
                        ? visibilitySource.lightIndex
                        : static_cast<std::size_t>(kInvalidRenderShadowIndex),
                    visibilityIndex));
        }

        result.passes.push_back(std::move(compiledPass));
    }

    return result;
}
