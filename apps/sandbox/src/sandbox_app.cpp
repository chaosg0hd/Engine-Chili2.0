#include "sandbox_app.hpp"

#include "app/app_capabilities.hpp"
#include "core/engine_core.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace
{
const wchar_t* ProjectionModeLabel(CameraProjectionModePrototype mode)
{
    switch (mode)
    {
    case CameraProjectionModePrototype::Orthographic:
        return L"orthographic";
    case CameraProjectionModePrototype::Perspective:
    default:
        return L"perspective";
    }
}

const wchar_t* FocusModeLabel(CameraFocusModePrototype mode)
{
    switch (mode)
    {
    case CameraFocusModePrototype::TargetBased:
        return L"target";
    case CameraFocusModePrototype::Auto:
        return L"auto";
    case CameraFocusModePrototype::ManualDistance:
    default:
        return L"manual";
    }
}

const wchar_t* CameraPurposeLabel(CameraPurposePrototype purpose)
{
    switch (purpose)
    {
    case CameraPurposePrototype::Cinematic:
        return L"cinematic";
    case CameraPurposePrototype::Debug:
        return L"debug";
    case CameraPurposePrototype::Preview:
        return L"preview";
    case CameraPurposePrototype::ShadowHelper:
        return L"shadow-helper";
    case CameraPurposePrototype::Gameplay:
    default:
        return L"gameplay";
    }
}

const wchar_t* EnabledLabel(bool enabled)
{
    return enabled ? L"on" : L"off";
}

const wchar_t* SecondaryQualityLabel(bool highQuality)
{
    return highQuality ? L"high" : L"low";
}

DerivedBounceFillSettings BuildDefaultDerivedBounceFillSettings()
{
    DerivedBounceFillSettings settings;
    settings.enabled = true;
    settings.bounceStrength = 0.10f;
    settings.environmentTint[0] = 1.0f;
    settings.environmentTint[1] = 1.0f;
    settings.environmentTint[2] = 1.0f;
    settings.shadowAwareBounce = true;
    return settings;
}

TracedIndirectSettings BuildDefaultTracedIndirectSettings()
{
    TracedIndirectSettings settings;
    settings.enabled = true;
    settings.bounceStrength = 0.22f;
    settings.maxTraceDistance = 7.0f;
    return settings;
}

DerivedBounceFillSettings BuildLowQualityDerivedBounceFillSettings()
{
    DerivedBounceFillSettings settings = BuildDefaultDerivedBounceFillSettings();
    settings.bounceStrength = 0.05f;
    return settings;
}

TracedIndirectSettings BuildLowQualityTracedIndirectSettings()
{
    TracedIndirectSettings settings;
    settings.enabled = true;
    settings.bounceStrength = 0.10f;
    settings.maxTraceDistance = 4.0f;
    return settings;
}

CameraPurposePrototype NextCameraPurpose(CameraPurposePrototype purpose)
{
    switch (purpose)
    {
    case CameraPurposePrototype::Gameplay:
        return CameraPurposePrototype::Cinematic;
    case CameraPurposePrototype::Cinematic:
        return CameraPurposePrototype::Debug;
    case CameraPurposePrototype::Debug:
        return CameraPurposePrototype::Preview;
    case CameraPurposePrototype::Preview:
        return CameraPurposePrototype::ShadowHelper;
    case CameraPurposePrototype::ShadowHelper:
    default:
        return CameraPurposePrototype::Gameplay;
    }
}

CameraPrototype BuildLightingLabDefaultCamera()
{
    CameraPrototype camera;
    camera.pose.position = Vector3(0.0f, 1.7f, 0.8f);
    camera.pose.target = Vector3(0.0f, 0.6f, 4.5f);
    camera.projection.fieldOfViewDegrees = 52.0f;
    camera.projection.nearPlane = 0.1f;
    camera.projection.farPlane = 200.0f;
    camera.exposure.exposure = 1.0f;
    return camera;
}

LightPrototype BuildLightingLabDefaultLight()
{
    LightPrototype light;
    light.emitter.kind = LightEmitterKind::Point;
    light.emitter.position = Vector3(0.0f, 2.8f, 6.6f);
    light.emitter.color = ColorPrototype::FromArgb(0xFFFFF1D0u);
    light.emitter.intensity = 3.4f;
    light.emitter.range = 13.0f;
    light.transport.kind = LightTransportKind::DirectAnalytic;
    light.visibility.kind = LightVisibilityKind::RasterShadowCubemap;
    light.visibility.policy.enabled = true;
    light.visibility.policy.method = LightVisibilityMethodPrototype::RasterCubemap;
    light.visibility.policy.refinement = LightVisibilityRefinementPrototype::None;
    light.visibility.policy.resolution = 512U;
    light.visibility.policy.depthBias = 0.02f;
    light.visibility.policy.nearPlane = 0.1f;
    light.visibility.policy.farPlane = light.emitter.range;
    light.visibility.policy.refinementInfluence = 0.0f;
    light.interaction.kind = LightInteractionKind::DirectBrdf;
    light.interaction.directBrdf = true;
    light.realization.kind = LightRealizationKind::RealtimeDynamic;
    light.enabled = true;
    return light;
}

MaterialPrototype BuildLightingLabStuccoRoomMaterial()
{
    MaterialPrototype material;
    material.baseLayer.albedo = ColorPrototype(1.0f, 1.0f, 1.0f, 1.0f);
    material.baseLayer.albedoAssetId = "library/materials/stucco/albedo.png";
    material.baseLayer.normalAssetId = "library/materials/stucco/normal.png";
    material.baseLayer.heightAssetId = "library/materials/stucco/height.png";
    material.baseLayer.albedoBlend = 0.0f;
    material.baseLayer.roughness = 0.82f;
    material.reflectivity = 0.0f;
    material.reflectionColor = material.baseLayer.albedo;
    material.brdf.ambientStrength = 0.07f;
    material.brdf.diffuseStrength = 0.94f;
    material.brdf.specularStrength = 0.06f;
    material.brdf.specularPower = 10.0f;
    return material;
}

MaterialPrototype BuildLightingLabStuccoFloorMaterial()
{
    MaterialPrototype material = BuildLightingLabStuccoRoomMaterial();
    material.baseLayer.roughness = 0.86f;
    material.brdf.ambientStrength = 0.06f;
    return material;
}

MaterialPrototype BuildLightingLabStuccoCubeMaterial()
{
    MaterialPrototype material = BuildLightingLabStuccoRoomMaterial();
    material.baseLayer.roughness = 0.34f;
    material.brdf.ambientStrength = 0.08f;
    material.brdf.diffuseStrength = 0.90f;
    material.brdf.specularStrength = 0.26f;
    material.brdf.specularPower = 22.0f;
    return material;
}

MaterialPrototype BuildLightingLabReflectiveCubeMaterial()
{
    MaterialPrototype material = BuildLightingLabStuccoRoomMaterial();
    material.baseLayer.albedo = ColorPrototype::FromArgb(0xFFD8E2F0u);
    material.baseLayer.roughness = 0.10f;
    material.reflectivity = 0.72f;
    material.reflectionColor = ColorPrototype::FromArgb(0xFFF4F7FFu);
    material.brdf.ambientStrength = 0.05f;
    material.brdf.diffuseStrength = 0.52f;
    material.brdf.specularStrength = 0.88f;
    material.brdf.specularPower = 72.0f;
    return material;
}

MaterialPrototype BuildLightingLabEmitterMaterial()
{
    MaterialPrototype material;
    material.baseLayer.albedo = ColorPrototype::FromArgb(0xFFFFF1D0u);
    material.reflectivity = 0.0f;
    material.reflectionColor = material.baseLayer.albedo;
    material.baseLayer.roughness = 0.12f;
    material.brdf.ambientStrength = 0.40f;
    material.brdf.diffuseStrength = 0.45f;
    material.brdf.specularStrength = 0.18f;
    material.brdf.specularPower = 16.0f;
    material.emissive.enabled = true;
    material.emissive.color = ColorPrototype::FromArgb(0xFFFFE7B3u);
    material.emissive.intensity = 2.8f;
    return material;
}
}

bool SandboxApp::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    ConfigureSandbox();
    AppCapabilities& capabilities = core.GetAppCapabilities();
    capabilities.logging->Info("SandboxApp: DX lighting sandbox ready.");
    ApplySecondaryLightingQuality(capabilities, true);
    InitializeControlButtons(capabilities);

    m_state = SandboxState{};
    ResetCamera();
    InitializeLightingLabMaterials();

    core.SetFrameCallback(
        [this](AppCapabilities& callbackCapabilities)
        {
            UpdateFrame(callbackCapabilities);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void SandboxApp::ConfigureSandbox()
{
    m_scene = SandboxPresetScenePrototype::LightingLab;
}

void SandboxApp::ApplySecondaryLightingQuality(AppCapabilities& capabilities, bool highQuality)
{
    m_state.highQualitySecondaryLighting = highQuality;
    capabilities.rendering->SetDerivedBounceFillSettings(
        highQuality ? BuildDefaultDerivedBounceFillSettings() : BuildLowQualityDerivedBounceFillSettings());
    capabilities.rendering->SetTracedIndirectSettings(
        highQuality ? BuildDefaultTracedIndirectSettings() : BuildLowQualityTracedIndirectSettings());

    std::ostringstream message;
    message << "[DX-LIGHTING] Secondary lighting quality set to "
            << (highQuality ? "high." : "low.");
    capabilities.logging->Info(message.str());
}

void SandboxApp::InitializeControlButtons(AppCapabilities& capabilities)
{
    const auto createButton =
        [&capabilities](const std::string& name, const wchar_t* text) -> IAppUi::NativeButtonHandle
        {
            NativeButtonDesc desc;
            desc.name = name;
            desc.text = text;
            desc.rect = NativeControlRect{ 0, 0, 72, 30 };
            desc.visible = true;
            desc.enabled = true;
            return capabilities.ui->CreateNativeButton(desc);
        };

    m_buttons.exposureDown = createButton("exp_minus", L"Exp -");
    m_buttons.exposureUp = createButton("exp_plus", L"Exp +");
    m_buttons.fovDown = createButton("fov_minus", L"FOV -");
    m_buttons.fovUp = createButton("fov_plus", L"FOV +");
    m_buttons.lightDown = createButton("light_minus", L"Light -");
    m_buttons.lightUp = createButton("light_plus", L"Light +");
    m_buttons.bounceDown = createButton("bounce_minus", L"Bounce -");
    m_buttons.bounceUp = createButton("bounce_plus", L"Bounce +");
    m_buttons.tracedDown = createButton("trace_minus", L"Trace -");
    m_buttons.tracedUp = createButton("trace_plus", L"Trace +");
    m_buttons.traceDistanceDown = createButton("dist_minus", L"Dist -");
    m_buttons.traceDistanceUp = createButton("dist_plus", L"Dist +");
    m_buttons.qualityToggle = createButton("quality_toggle", L"Quality");
    LayoutControlButtons(capabilities);
}

void SandboxApp::LayoutControlButtons(AppCapabilities& capabilities)
{
    const int windowWidth = capabilities.window->GetWindowWidth();
    const int windowHeight = capabilities.window->GetWindowHeight();
    if (windowWidth <= 0 || windowHeight <= 0)
    {
        return;
    }

    constexpr int buttonWidth = 82;
    constexpr int buttonHeight = 30;
    constexpr int gap = 8;
    constexpr int leftMargin = 12;
    constexpr int bottomMargin = 12;
    const int y = windowHeight - bottomMargin - buttonHeight;
    const int secondRowY = y - gap - buttonHeight;

    const IAppUi::NativeButtonHandle orderedButtons[] =
    {
        m_buttons.exposureDown,
        m_buttons.exposureUp,
        m_buttons.fovDown,
        m_buttons.fovUp,
        m_buttons.lightDown,
        m_buttons.lightUp,
        m_buttons.bounceDown,
        m_buttons.bounceUp,
        m_buttons.tracedDown,
        m_buttons.tracedUp,
        m_buttons.traceDistanceDown,
        m_buttons.traceDistanceUp
    };

    int x = leftMargin;
    for (IAppUi::NativeButtonHandle handle : orderedButtons)
    {
        if (handle == 0U)
        {
            continue;
        }

        capabilities.ui->SetNativeButtonBounds(handle, NativeControlRect{ x, y, buttonWidth, buttonHeight });
        x += buttonWidth + gap;
    }

    if (m_buttons.qualityToggle != 0U)
    {
        capabilities.ui->SetNativeButtonBounds(
            m_buttons.qualityToggle,
            NativeControlRect{ leftMargin, secondRowY, buttonWidth + 18, buttonHeight });
    }
}

void SandboxApp::InitializeLightingLabMaterials()
{
    m_floorMaterial = BuildLightingLabStuccoFloorMaterial();
    m_roomMaterial = BuildLightingLabStuccoRoomMaterial();
    m_cubeMaterial = BuildLightingLabStuccoCubeMaterial();
    m_reflectiveCubeMaterial = BuildLightingLabReflectiveCubeMaterial();
    m_emitterMaterial = BuildLightingLabEmitterMaterial();
}

void SandboxApp::ResetCamera()
{
    m_state.camera = BuildLightingLabDefaultCamera();
    m_state.primaryLight = BuildLightingLabDefaultLight();
}

void SandboxApp::UpdateFrame(AppCapabilities& capabilities)
{
    LayoutControlButtons(capabilities);
    UpdateLogic(capabilities);

    capabilities.rendering->ClearFrame(0xFF0A0D14u);
    capabilities.rendering->SubmitFrame(BuildFrame());
    capabilities.window->SetOverlayText(BuildOverlayText(capabilities));
}

void SandboxApp::UpdateLogic(AppCapabilities& capabilities)
{
    const float deltaTime = static_cast<float>(capabilities.window->GetDeltaTime());
    DerivedBounceFillSettings bounceSettings = capabilities.rendering->GetDerivedBounceFillSettings();
    TracedIndirectSettings tracedSettings = capabilities.rendering->GetTracedIndirectSettings();
    bool bounceSettingsChanged = false;
    bool tracedSettingsChanged = false;

    const auto commitBounceSettings =
        [&capabilities, &bounceSettings, &bounceSettingsChanged]()
        {
            if (bounceSettingsChanged)
            {
                capabilities.rendering->SetDerivedBounceFillSettings(bounceSettings);
            }
        };

    const auto commitTracedSettings =
        [&capabilities, &tracedSettings, &tracedSettingsChanged]()
        {
            if (tracedSettingsChanged)
            {
                capabilities.rendering->SetTracedIndirectSettings(tracedSettings);
            }
        };

    if (capabilities.window->WasKeyPressed(' '))
    {
        m_state.rotationPaused = !m_state.rotationPaused;
        capabilities.logging->Info(m_state.rotationPaused
            ? "[DX-LIGHTING] Animation paused."
            : "[DX-LIGHTING] Animation resumed.");
    }

    if (capabilities.window->WasKeyPressed('O'))
    {
        m_state.orbitEnabled = !m_state.orbitEnabled;
        capabilities.logging->Info(m_state.orbitEnabled
            ? "[DX-LIGHTING] Camera orbit enabled."
            : "[DX-LIGHTING] Camera orbit disabled.");
    }

    if (capabilities.window->WasKeyPressed('R'))
    {
        m_state = SandboxState{};
        ResetCamera();
        bounceSettings = BuildDefaultDerivedBounceFillSettings();
        tracedSettings = BuildDefaultTracedIndirectSettings();
        bounceSettingsChanged = true;
        tracedSettingsChanged = true;
        capabilities.logging->Info("[DX-LIGHTING] Sandbox state reset.");
    }

    if (capabilities.window->WasKeyPressed(VK_F2) ||
        capabilities.ui->ConsumeNativeButtonPressed(m_buttons.qualityToggle))
    {
        ApplySecondaryLightingQuality(capabilities, !m_state.highQualitySecondaryLighting);
        bounceSettings = capabilities.rendering->GetDerivedBounceFillSettings();
        tracedSettings = capabilities.rendering->GetTracedIndirectSettings();
        bounceSettingsChanged = false;
        tracedSettingsChanged = false;
    }

    if (capabilities.window->WasKeyPressed(VK_OEM_MINUS))
    {
        m_state.camera.exposure.exposure = std::max(0.1f, m_state.camera.exposure.exposure - 0.1f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera exposure set to " << m_state.camera.exposure.exposure;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed(VK_OEM_PLUS))
    {
        m_state.camera.exposure.exposure = std::min(4.0f, m_state.camera.exposure.exposure + 0.1f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera exposure set to " << m_state.camera.exposure.exposure;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.exposureDown))
    {
        m_state.camera.exposure.exposure = std::max(0.1f, m_state.camera.exposure.exposure - 0.1f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera exposure set to " << m_state.camera.exposure.exposure;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.exposureUp))
    {
        m_state.camera.exposure.exposure = std::min(4.0f, m_state.camera.exposure.exposure + 0.1f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera exposure set to " << m_state.camera.exposure.exposure;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed(VK_OEM_COMMA))
    {
        m_state.camera.exposure.exposureCompensation =
            std::max(-4.0f, m_state.camera.exposure.exposureCompensation - 0.25f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Exposure compensation set to "
                << m_state.camera.exposure.exposureCompensation
                << " EV";
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed(VK_OEM_PERIOD))
    {
        m_state.camera.exposure.exposureCompensation =
            std::min(4.0f, m_state.camera.exposure.exposureCompensation + 0.25f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Exposure compensation set to "
                << m_state.camera.exposure.exposureCompensation
                << " EV";
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed(VK_OEM_4))
    {
        m_state.camera.projection.fieldOfViewDegrees =
            std::max(20.0f, m_state.camera.projection.fieldOfViewDegrees - 2.0f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera FOV set to "
                << m_state.camera.projection.fieldOfViewDegrees
                << " degrees.";
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed(VK_OEM_6))
    {
        m_state.camera.projection.fieldOfViewDegrees =
            std::min(110.0f, m_state.camera.projection.fieldOfViewDegrees + 2.0f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera FOV set to "
                << m_state.camera.projection.fieldOfViewDegrees
                << " degrees.";
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.fovDown))
    {
        m_state.camera.projection.fieldOfViewDegrees =
            std::max(20.0f, m_state.camera.projection.fieldOfViewDegrees - 2.0f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera FOV set to "
                << m_state.camera.projection.fieldOfViewDegrees
                << " degrees.";
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.fovUp))
    {
        m_state.camera.projection.fieldOfViewDegrees =
            std::min(110.0f, m_state.camera.projection.fieldOfViewDegrees + 2.0f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera FOV set to "
                << m_state.camera.projection.fieldOfViewDegrees
                << " degrees.";
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('P'))
    {
        m_state.camera.projection.mode =
            m_state.camera.projection.mode == CameraProjectionModePrototype::Perspective
                ? CameraProjectionModePrototype::Orthographic
                : CameraProjectionModePrototype::Perspective;
        std::ostringstream message;
        message << "[DX-LIGHTING] Projection mode set to "
                << (m_state.camera.projection.mode == CameraProjectionModePrototype::Perspective
                    ? "perspective"
                    : "orthographic")
                << '.';
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('9'))
    {
        m_state.camera.projection.orthographicHeight =
            std::max(2.0f, m_state.camera.projection.orthographicHeight - 0.5f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Orthographic height set to "
                << m_state.camera.projection.orthographicHeight
                << '.';
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('0'))
    {
        m_state.camera.projection.orthographicHeight =
            std::min(40.0f, m_state.camera.projection.orthographicHeight + 0.5f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Orthographic height set to "
                << m_state.camera.projection.orthographicHeight
                << '.';
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('C'))
    {
        if (m_state.camera.projection.aspectRatioOverride > 0.000001f)
        {
            if (std::fabs(m_state.camera.projection.aspectRatioOverride - 1.7777778f) < 0.01f)
            {
                m_state.camera.projection.aspectRatioOverride = 1.0f;
                capabilities.logging->Info("[DX-LIGHTING] Aspect ratio override set to 1:1.");
            }
            else
            {
                m_state.camera.projection.aspectRatioOverride = 0.0f;
                capabilities.logging->Info("[DX-LIGHTING] Aspect ratio override cleared to window aspect.");
            }
        }
        else
        {
            m_state.camera.projection.aspectRatioOverride = 1.7777778f;
            capabilities.logging->Info("[DX-LIGHTING] Aspect ratio override set to 16:9.");
        }
    }

    if (capabilities.window->WasKeyPressed('V'))
    {
        m_state.camera.focus.focusDistance = std::max(0.5f, m_state.camera.focus.focusDistance - 0.5f);
        m_state.camera.focus.mode = CameraFocusModePrototype::ManualDistance;
        std::ostringstream message;
        message << "[DX-LIGHTING] Focus distance set to "
                << m_state.camera.focus.focusDistance
                << " meters.";
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('B'))
    {
        m_state.camera.focus.focusDistance = std::min(50.0f, m_state.camera.focus.focusDistance + 0.5f);
        m_state.camera.focus.mode = CameraFocusModePrototype::ManualDistance;
        std::ostringstream message;
        message << "[DX-LIGHTING] Focus distance set to "
                << m_state.camera.focus.focusDistance
                << " meters.";
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('F'))
    {
        if (m_state.camera.focus.mode == CameraFocusModePrototype::ManualDistance)
        {
            m_state.camera.focus.mode = CameraFocusModePrototype::TargetBased;
            m_state.camera.focus.focusTarget = m_state.camera.GetTargetPosition();
            m_state.camera.focus.focusDistance =
                Length(m_state.camera.focus.focusTarget - m_state.camera.GetWorldPosition());
            capabilities.logging->Info("[DX-LIGHTING] Focus mode set to target-based.");
        }
        else
        {
            m_state.camera.focus.mode = CameraFocusModePrototype::ManualDistance;
            capabilities.logging->Info("[DX-LIGHTING] Focus mode set to manual.");
        }
    }

    if (capabilities.window->WasKeyPressed('T'))
    {
        m_state.camera.purpose = NextCameraPurpose(m_state.camera.purpose);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera purpose cycled.";
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('J'))
    {
        m_state.primaryLight.emitter.intensity = std::max(0.1f, m_state.primaryLight.emitter.intensity - 0.5f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light intensity set to " << m_state.primaryLight.emitter.intensity;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('K'))
    {
        m_state.primaryLight.emitter.intensity = std::min(20.0f, m_state.primaryLight.emitter.intensity + 0.5f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light intensity set to " << m_state.primaryLight.emitter.intensity;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.lightDown))
    {
        m_state.primaryLight.emitter.intensity = std::max(0.1f, m_state.primaryLight.emitter.intensity - 0.5f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light intensity set to " << m_state.primaryLight.emitter.intensity;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.lightUp))
    {
        m_state.primaryLight.emitter.intensity = std::min(20.0f, m_state.primaryLight.emitter.intensity + 0.5f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light intensity set to " << m_state.primaryLight.emitter.intensity;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('N'))
    {
        m_state.primaryLight.emitter.range = std::max(2.0f, m_state.primaryLight.emitter.range - 1.0f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light range set to " << m_state.primaryLight.emitter.range;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('M'))
    {
        m_state.primaryLight.emitter.range = std::min(40.0f, m_state.primaryLight.emitter.range + 1.0f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light range set to " << m_state.primaryLight.emitter.range;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('Y'))
    {
        bounceSettings.enabled = !bounceSettings.enabled;
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Derived bounce fill "
                << (bounceSettings.enabled ? "enabled." : "disabled.");
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('U'))
    {
        bounceSettings.bounceStrength = std::max(0.0f, bounceSettings.bounceStrength - 0.02f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce strength set to " << bounceSettings.bounceStrength;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('I'))
    {
        bounceSettings.bounceStrength = std::min(1.0f, bounceSettings.bounceStrength + 0.02f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce strength set to " << bounceSettings.bounceStrength;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.bounceDown))
    {
        bounceSettings.bounceStrength = std::max(0.0f, bounceSettings.bounceStrength - 0.02f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce strength set to " << bounceSettings.bounceStrength;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.bounceUp))
    {
        bounceSettings.bounceStrength = std::min(1.0f, bounceSettings.bounceStrength + 0.02f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce strength set to " << bounceSettings.bounceStrength;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('H'))
    {
        bounceSettings.shadowAwareBounce = !bounceSettings.shadowAwareBounce;
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Shadow-aware bounce "
                << (bounceSettings.shadowAwareBounce ? "enabled." : "disabled.");
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('1'))
    {
        bounceSettings.environmentTint[0] = std::max(0.0f, bounceSettings.environmentTint[0] - 0.1f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce tint R set to " << bounceSettings.environmentTint[0];
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('2'))
    {
        bounceSettings.environmentTint[0] = std::min(2.0f, bounceSettings.environmentTint[0] + 0.1f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce tint R set to " << bounceSettings.environmentTint[0];
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('3'))
    {
        bounceSettings.environmentTint[1] = std::max(0.0f, bounceSettings.environmentTint[1] - 0.1f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce tint G set to " << bounceSettings.environmentTint[1];
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('4'))
    {
        bounceSettings.environmentTint[1] = std::min(2.0f, bounceSettings.environmentTint[1] + 0.1f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce tint G set to " << bounceSettings.environmentTint[1];
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('5'))
    {
        bounceSettings.environmentTint[2] = std::max(0.0f, bounceSettings.environmentTint[2] - 0.1f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce tint B set to " << bounceSettings.environmentTint[2];
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('6'))
    {
        bounceSettings.environmentTint[2] = std::min(2.0f, bounceSettings.environmentTint[2] + 0.1f);
        bounceSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Bounce tint B set to " << bounceSettings.environmentTint[2];
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('G'))
    {
        tracedSettings.enabled = !tracedSettings.enabled;
        tracedSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Traced indirect "
                << (tracedSettings.enabled ? "enabled." : "disabled.");
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('7'))
    {
        tracedSettings.bounceStrength = std::max(0.0f, tracedSettings.bounceStrength - 0.05f);
        tracedSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Traced bounce strength set to " << tracedSettings.bounceStrength;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('8'))
    {
        tracedSettings.bounceStrength = std::min(2.0f, tracedSettings.bounceStrength + 0.05f);
        tracedSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Traced bounce strength set to " << tracedSettings.bounceStrength;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.tracedDown))
    {
        tracedSettings.bounceStrength = std::max(0.0f, tracedSettings.bounceStrength - 0.05f);
        tracedSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Traced bounce strength set to " << tracedSettings.bounceStrength;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.tracedUp))
    {
        tracedSettings.bounceStrength = std::min(2.0f, tracedSettings.bounceStrength + 0.05f);
        tracedSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Traced bounce strength set to " << tracedSettings.bounceStrength;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('L'))
    {
        tracedSettings.maxTraceDistance = std::max(1.0f, tracedSettings.maxTraceDistance - 1.0f);
        tracedSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Traced max distance set to " << tracedSettings.maxTraceDistance;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed(VK_OEM_1))
    {
        tracedSettings.maxTraceDistance = std::min(30.0f, tracedSettings.maxTraceDistance + 1.0f);
        tracedSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Traced max distance set to " << tracedSettings.maxTraceDistance;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.traceDistanceDown))
    {
        tracedSettings.maxTraceDistance = std::max(1.0f, tracedSettings.maxTraceDistance - 1.0f);
        tracedSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Traced max distance set to " << tracedSettings.maxTraceDistance;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.ui->ConsumeNativeButtonPressed(m_buttons.traceDistanceUp))
    {
        tracedSettings.maxTraceDistance = std::min(30.0f, tracedSettings.maxTraceDistance + 1.0f);
        tracedSettingsChanged = true;
        std::ostringstream message;
        message << "[DX-LIGHTING] Traced max distance set to " << tracedSettings.maxTraceDistance;
        capabilities.logging->Info(message.str());
    }

    if (!m_state.orbitEnabled)
    {
        const float moveSpeed = capabilities.window->IsKeyDown(VK_SHIFT) ? 9.0f : 4.5f;
        const float orbitSpeed = 1.6f;
        const float zoomSpeed = 8.0f;
        const float moveDistance = moveSpeed * deltaTime;
        const float orbitStep = orbitSpeed * deltaTime;

        if (capabilities.window->IsKeyDown('W'))
        {
            m_state.camera.MoveForward(moveDistance);
        }
        if (capabilities.window->IsKeyDown('S'))
        {
            m_state.camera.MoveForward(-moveDistance);
        }
        if (capabilities.window->IsKeyDown('A'))
        {
            m_state.camera.MoveRight(-moveDistance);
        }
        if (capabilities.window->IsKeyDown('D'))
        {
            m_state.camera.MoveRight(moveDistance);
        }
        if (capabilities.window->IsKeyDown('Q'))
        {
            m_state.camera.MoveUp(-moveDistance);
        }
        if (capabilities.window->IsKeyDown('E'))
        {
            m_state.camera.MoveUp(moveDistance);
        }

        const Vector3 focusPoint = m_state.camera.GetTargetPosition();
        if (capabilities.window->IsKeyDown(VK_LEFT))
        {
            m_state.camera.OrbitAround(focusPoint, -orbitStep, 0.0f);
        }
        if (capabilities.window->IsKeyDown(VK_RIGHT))
        {
            m_state.camera.OrbitAround(focusPoint, orbitStep, 0.0f);
        }
        if (capabilities.window->IsKeyDown(VK_UP))
        {
            m_state.camera.OrbitAround(focusPoint, 0.0f, orbitStep * 0.75f);
        }
        if (capabilities.window->IsKeyDown(VK_DOWN))
        {
            m_state.camera.OrbitAround(focusPoint, 0.0f, -orbitStep * 0.75f);
        }
        if (capabilities.window->IsKeyDown('Z'))
        {
            const Vector3 forward = m_state.camera.GetForward();
            const float currentDistance = Length(m_state.camera.GetTargetPosition() - m_state.camera.GetWorldPosition());
            const float nextDistance = std::max(1.5f, currentDistance - (zoomSpeed * deltaTime));
            m_state.camera.pose.position = m_state.camera.GetTargetPosition() - (forward * nextDistance);
        }
        if (capabilities.window->IsKeyDown('X'))
        {
            const Vector3 forward = m_state.camera.GetForward();
            const float currentDistance = Length(m_state.camera.GetTargetPosition() - m_state.camera.GetWorldPosition());
            const float nextDistance = std::min(40.0f, currentDistance + (zoomSpeed * deltaTime));
            m_state.camera.pose.position = m_state.camera.GetTargetPosition() - (forward * nextDistance);
        }
    }

    if (!m_state.rotationPaused)
    {
        m_state.liveSceneTime = capabilities.window->GetTotalTime() - m_state.pausedSceneTime;
    }
    else
    {
        m_state.pausedSceneTime = capabilities.window->GetTotalTime() - m_state.liveSceneTime;
    }

    commitBounceSettings();
    commitTracedSettings();
}

FramePrototype SandboxApp::BuildFrame() const
{
    SandboxScenePresetOptionsPrototype options;
    options.totalTime = m_state.liveSceneTime;
    options.rotationPaused = m_state.rotationPaused;
    options.cameraOrbitEnabled = m_state.orbitEnabled;
    options.cameraOverrideEnabled = !m_state.orbitEnabled;
    options.highQualitySecondaryLighting = m_state.highQualitySecondaryLighting;
    options.cameraControlCamera = m_state.camera;
    options.primaryLight = m_state.primaryLight;
    options.floorMaterial = &m_floorMaterial;
    options.roomMaterial = &m_roomMaterial;
    options.cubeMaterial = &m_cubeMaterial;
    options.reflectiveCubeMaterial = &m_reflectiveCubeMaterial;
    options.emitterMaterial = &m_emitterMaterial;
    return SandboxScenePresetCompiler::BuildFrame(m_scene, options);
}

std::wstring SandboxApp::BuildOverlayText(const AppCapabilities& capabilities) const
{
    const DerivedBounceFillSettings bounceSettings = capabilities.rendering->GetDerivedBounceFillSettings();
    const TracedIndirectSettings tracedSettings = capabilities.rendering->GetTracedIndirectSettings();
    std::wostringstream overlay;
    overlay << L"DX LIGHTING LAB\n";
    overlay << L"backend: ";
    overlay << L"engine-runtime\n";
    overlay << L"scene: lighting lab\n";
    overlay << L"items: " << capabilities.rendering->GetSubmittedItemCount() << L"\n";
    overlay << L"frame: " << capabilities.rendering->GetFrameWidth() << L"x" << capabilities.rendering->GetFrameHeight() << L"\n";
    overlay << L"dt: " << capabilities.window->GetDeltaTime() << L" s\n";
    overlay << L"time: " << m_state.liveSceneTime << L" s\n";
    overlay << L"camera: " << (m_state.orbitEnabled ? L"orbit" : L"manual") << L"\n";
    overlay << L"exposure: " << m_state.camera.exposure.exposure << L"\n";
    overlay << L"exposure ev: " << m_state.camera.exposure.exposureCompensation << L"\n";
    overlay << L"projection: " << ProjectionModeLabel(m_state.camera.projection.mode) << L"\n";
    overlay << L"fov: " << m_state.camera.projection.fieldOfViewDegrees << L" deg\n";
    overlay << L"ortho height: " << m_state.camera.projection.orthographicHeight << L"\n";
    overlay << L"aspect override: ";
    if (m_state.camera.projection.aspectRatioOverride > 0.000001f)
    {
        overlay << m_state.camera.projection.aspectRatioOverride;
    }
    else
    {
        overlay << L"window";
    }
    overlay << L"\n";
    overlay << L"focus: " << FocusModeLabel(m_state.camera.focus.mode)
            << L" @ " << m_state.camera.focus.focusDistance << L" m\n";
    overlay << L"purpose: " << CameraPurposeLabel(m_state.camera.purpose) << L"\n";
    overlay << L"light intensity: " << m_state.primaryLight.emitter.intensity << L"\n";
    overlay << L"light range: " << m_state.primaryLight.emitter.range << L"\n";
    overlay << L"secondary quality: " << SecondaryQualityLabel(m_state.highQualitySecondaryLighting) << L"\n";
    overlay << L"bounce: " << EnabledLabel(bounceSettings.enabled)
            << L" strength=" << bounceSettings.bounceStrength
            << L" shadow-aware=" << EnabledLabel(bounceSettings.shadowAwareBounce) << L"\n";
    overlay << L"bounce tint: "
            << bounceSettings.environmentTint[0] << L", "
            << bounceSettings.environmentTint[1] << L", "
            << bounceSettings.environmentTint[2] << L"\n";
    overlay << L"traced indirect: " << EnabledLabel(tracedSettings.enabled)
            << L" strength=" << tracedSettings.bounceStrength
            << L" maxDist=" << tracedSettings.maxTraceDistance << L"\n";
    overlay << L"animation: " << (m_state.rotationPaused ? L"paused" : L"running") << L"\n";
    overlay << L"materials: sandbox-owned lighting lab set\n";
    overlay << L"[space] pause  [O] orbit/manual  [R] reset  [-/+] exposure  [</>] EV\n";
    overlay << L"[[/]] fov  [P] proj  [9/0] ortho size  [C] aspect\n";
    overlay << L"[F] focus mode  [V/B] focus dist  [T] purpose\n";
    overlay << L"[J/K] light intensity  [N/M] light range\n";
    overlay << L"[F2] secondary quality low/high\n";
    overlay << L"[Y] bounce on/off  [U/I] bounce strength  [H] shadow-aware bounce\n";
    overlay << L"[1/2] tint R  [3/4] tint G  [5/6] tint B\n";
    overlay << L"[G] traced on/off  [7/8] traced strength  [L/;] traced distance\n";
    overlay << L"[WASD/QE] move  [arrows] orbit  [Z/X] zoom  [Shift] faster  [Esc] quit";
    return overlay.str();
}
