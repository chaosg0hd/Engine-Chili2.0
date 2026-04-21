#pragma once

#include "../entity/scene/camera.hpp"
#include "../presentation/frame.hpp"

enum class SandboxPresetScenePrototype : unsigned char
{
    VisibilityRoom = 0,
    PrototypeGrid,
    CameraControlLab
};

struct SandboxScenePresetOptionsPrototype
{
    double totalTime = 0.0;
    bool rotationPaused = false;
    bool cameraOrbitEnabled = true;
    CameraPrototype cameraControlCamera;
};

class SandboxScenePresetCompiler
{
public:
    static FramePrototype BuildFrame(
        SandboxPresetScenePrototype scene,
        const SandboxScenePresetOptionsPrototype& options);

    static unsigned int CountFrameItems(const FramePrototype& frame);
};
