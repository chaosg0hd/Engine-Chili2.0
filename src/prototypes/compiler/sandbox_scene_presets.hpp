#pragma once

#include "../entity/appearance/light.hpp"
#include "../entity/appearance/material.hpp"
#include "../entity/scene/camera.hpp"
#include "../presentation/frame.hpp"

enum class SandboxPresetScenePrototype : unsigned char
{
    VisibilityRoom = 0,
    PrototypeGrid,
    CameraControlLab,
    LightingLab
};

struct SandboxScenePresetOptionsPrototype
{
    double totalTime = 0.0;
    bool rotationPaused = false;
    bool cameraOrbitEnabled = true;
    bool cameraOverrideEnabled = false;
    CameraPrototype cameraControlCamera;
    SceneLightPrototype primarySceneLight;
    const MaterialPrototype* floorMaterial = nullptr;
    const MaterialPrototype* roomMaterial = nullptr;
    const MaterialPrototype* cubeMaterial = nullptr;
    const MaterialPrototype* emitterMaterial = nullptr;
};

class SandboxScenePresetCompiler
{
public:
    static FramePrototype BuildFrame(
        SandboxPresetScenePrototype scene,
        const SandboxScenePresetOptionsPrototype& options);

    static unsigned int CountFrameItems(const FramePrototype& frame);
};
