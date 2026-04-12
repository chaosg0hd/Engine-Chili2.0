#pragma once

#include "prototypes/entity/object/mesh.hpp"
#include "prototypes/entity/scene/camera.hpp"
#include "prototypes/math/math.hpp"
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
        ToggleCameraOrbit,
        SelectVisibilityRoom,
        SelectPrototypeGrid,
        SelectCameraControlLab,
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        Count
    };

    enum class SandboxScene
    {
        VisibilityRoom = 0,
        PrototypeGrid,
        CameraControlLab
    };

    struct SandboxState
    {
        unsigned long long frameCount = 0;
        double totalTime = 0.0;
        double pulse = 0.0;
        unsigned int objectCount = 0;
        bool rotationPaused = false;
        bool cameraOrbitEnabled = true;
        SandboxScene activeScene = SandboxScene::CameraControlLab;
        CameraPrototype cameraControlCamera;
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
    void UpdateCameraControlLab(EngineCore& core);
    FramePrototype BuildActiveFrame() const;
    FramePrototype BuildVisibilityRoomFrame() const;
    FramePrototype BuildPrototypeGridFrame() const;
    FramePrototype BuildCameraControlLabFrame() const;
    std::wstring BuildPresentationOverlay(const EngineCore& core) const;
    void InitializeSandboxLog(EngineCore& core);
    void AppendSandboxLogLine(const std::string& line);
    void FlushSandboxLog(EngineCore& core, bool force);
    bool TrySelectScene(EngineCore& core, SandboxScene scene, const char* reason);
    const char* GetSceneName(SandboxScene scene) const;
    std::wstring GetActiveSceneNameWide() const;
    bool IsActionPressed(EngineCore& core, SandboxAction action) const;
    bool IsActionDown(EngineCore& core, SandboxAction action) const;
    const InputBinding& GetBinding(SandboxAction action) const;
    ViewPrototype BuildDefaultSceneView() const;
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
    std::string m_sandboxLogPath = "logs/sandbox_3d_visibility.txt";
    std::string m_sandboxLogBuffer;
    double m_nextSandboxLogFlushTime = 0.0;
};
