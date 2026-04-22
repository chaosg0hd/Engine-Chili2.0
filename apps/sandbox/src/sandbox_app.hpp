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
        double pausedSceneTime = 0.0;
        double liveSceneTime = 0.0;
        CameraPrototype camera;
        SceneLightPrototype primarySceneLight;
    };

private:
    void UpdateFrame(AppCapabilities& capabilities);
    void UpdateLogic(AppCapabilities& capabilities);
    void ConfigureSandbox();
    bool InitializeMaterialPrototypes(const AppCapabilities& capabilities);
    void ResetCamera();
    FramePrototype BuildFrame() const;
    std::wstring BuildOverlayText(const AppCapabilities& capabilities) const;

private:
    SandboxState m_state;
    SandboxPresetScenePrototype m_scene = SandboxPresetScenePrototype::LightingLab;
    MaterialPrototype m_floorMaterial;
    MaterialPrototype m_roomMaterial;
    MaterialPrototype m_cubeMaterial;
    MaterialPrototype m_emitterMaterial;
};
