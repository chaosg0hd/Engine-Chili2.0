#pragma once

#include "prototypes/entity/scene/camera.hpp"
#include "prototypes/math/math.hpp"
#include "prototypes/presentation/frame.hpp"
#include "prototypes/presentation/view.hpp"

#include <cstdint>
#include <string>

class EngineCore;

class SandboxApp
{
public:
    bool Run();

private:
    struct SandboxState
    {
        unsigned long long frameCount = 0;
        double totalTime = 0.0;
        double aspectRatio = 1.7777777777777777;
        CameraPrototype camera;
        Vector3 lightOrigin = Vector3(0.0f, 1.2f, -1.5f);
        Vector3 lightDirection = Vector3(0.0f, -0.08f, 1.0f);
        unsigned int raycastCount = 32U;
        bool animateLight = true;
    };

private:
    void UpdateFrame(EngineCore& core);
    void UpdateLogic(EngineCore& core);
    void UpdateCameraMovement(EngineCore& core);
    void UpdateLightControls(EngineCore& core);
    void ResetLab();
    FramePrototype BuildLightLabFrame() const;
    std::wstring BuildOverlay(const EngineCore& core) const;
    void AddCube(
        ViewPrototype& view,
        const Vector3& position,
        const Vector3& scale,
        const Vector3& rotation,
        std::uint32_t color) const;
    void AddRayMarker(
        ViewPrototype& view,
        const Vector3& origin,
        const Vector3& direction,
        float length,
        float thickness,
        std::uint32_t color) const;

private:
    SandboxState m_state;
};
