#pragma once

#include "app/app_capabilities.hpp"
#include "prototypes/compiler/sandbox_scene_presets.hpp"
#include "prototypes/entity/appearance/light.hpp"
#include "prototypes/entity/scene/camera.hpp"
#include "prototypes/presentation/frame.hpp"

#include <string>
#include <vector>

class SandboxApp
{
public:
    bool Run();

private:
    struct SandboxState
    {
        bool rotationPaused = false;
        bool orbitEnabled = true;
        bool highQualitySecondaryLighting = true;
        double pausedSceneTime = 0.0;
        double liveSceneTime = 0.0;
        CameraPrototype camera;
        LightPrototype primaryLight;
    };

private:
    struct ControlButtons
    {
        IAppUi::NativeButtonHandle exposureDown = 0U;
        IAppUi::NativeButtonHandle exposureUp = 0U;
        IAppUi::NativeButtonHandle fovDown = 0U;
        IAppUi::NativeButtonHandle fovUp = 0U;
        IAppUi::NativeButtonHandle lightDown = 0U;
        IAppUi::NativeButtonHandle lightUp = 0U;
        IAppUi::NativeButtonHandle bounceDown = 0U;
        IAppUi::NativeButtonHandle bounceUp = 0U;
        IAppUi::NativeButtonHandle tracedDown = 0U;
        IAppUi::NativeButtonHandle tracedUp = 0U;
        IAppUi::NativeButtonHandle traceDistanceDown = 0U;
        IAppUi::NativeButtonHandle traceDistanceUp = 0U;
        IAppUi::NativeButtonHandle qualityToggle = 0U;
    };

private:
    void ApplySecondaryLightingQuality(AppCapabilities& capabilities, bool highQuality);
    void InitializeControlButtons(AppCapabilities& capabilities);
    void LayoutControlButtons(AppCapabilities& capabilities);
    void UpdateFrame(AppCapabilities& capabilities);
    void UpdateLogic(AppCapabilities& capabilities);
    void ConfigureSandbox();
    void InitializeLightingLabMaterials();
    void ResetCamera();
    FramePrototype BuildFrame() const;
    std::wstring BuildOverlayText(const AppCapabilities& capabilities) const;

private:
    SandboxState m_state;
    SandboxPresetScenePrototype m_scene = SandboxPresetScenePrototype::LightingLab;
    MaterialPrototype m_floorMaterial;
    MaterialPrototype m_roomMaterial;
    MaterialPrototype m_cubeMaterial;
    MaterialPrototype m_reflectiveCubeMaterial;
    MaterialPrototype m_emitterMaterial;
    ControlButtons m_buttons;
};
