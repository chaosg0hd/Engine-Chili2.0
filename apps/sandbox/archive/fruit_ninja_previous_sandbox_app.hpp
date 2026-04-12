#pragma once

#include "prototypes/entity/object/mesh.hpp"
#include "prototypes/entity/scene/camera.hpp"
#include "prototypes/presentation/frame.hpp"
#include "prototypes/presentation/view.hpp"

#include <array>
#include <string>

class EngineCore;

class SandboxApp
{
public:
    bool Run();

private:
    enum class SandboxAction
    {
        ToggleRotation = 0,
        ResetCamera,
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        Count
    };

    struct SandboxState
    {
        unsigned long long frameCount = 0;
        double totalTime = 0.0;
        double pulse = 0.0;
        unsigned int objectCount = 0;
        bool rotationPaused = false;
        CameraPrototype camera;
    };

    struct InputBinding
    {
        SandboxAction action = SandboxAction::ToggleRotation;
        unsigned char key = 0;
        const wchar_t* label = L"";
        const wchar_t* keyLabel = L"";
    };

private:
    void UpdateFrame(EngineCore& core);
    void UpdateLogic(EngineCore& core);
    void UpdateCamera(EngineCore& core);
    void ResetCamera();
    FramePrototype BuildLabFrame() const;
    std::wstring BuildOverlay(const EngineCore& core) const;
    bool IsActionPressed(EngineCore& core, SandboxAction action) const;
    bool IsActionDown(EngineCore& core, SandboxAction action) const;
    const InputBinding& GetBinding(SandboxAction action) const;
    unsigned int CountFrameItems(const FramePrototype& frame) const;
    void AddObject(
        ViewPrototype& view,
        BuiltInMeshKind meshKind,
        const Vector3& translation,
        const Vector3& scale,
        const Vector3& rotation,
        std::uint32_t color) const;

private:
    static const std::array<InputBinding, static_cast<std::size_t>(SandboxAction::Count)> kInputBindings;

private:
    SandboxState m_state;
};
