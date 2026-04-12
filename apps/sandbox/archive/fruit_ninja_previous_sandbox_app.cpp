#include "sandbox_app.hpp"

#include "core/engine_core.hpp"
#include "prototypes/entity/scene/infinite_plane.hpp"
#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"

#include <cmath>
#include <string>

const std::array<SandboxApp::InputBinding, static_cast<std::size_t>(SandboxApp::SandboxAction::Count)>
    SandboxApp::kInputBindings =
{{
    { SandboxAction::ToggleRotation, 9, L"Toggle Rotation", L"Tab" },
    { SandboxAction::ResetCamera, 'R', L"Reset Camera", L"R" },
    { SandboxAction::MoveForward, 'W', L"Move Forward", L"W" },
    { SandboxAction::MoveBackward, 'S', L"Move Backward", L"S" },
    { SandboxAction::MoveLeft, 'A', L"Move Left", L"A" },
    { SandboxAction::MoveRight, 'D', L"Move Right", L"D" },
    { SandboxAction::MoveUp, 'E', L"Move Up", L"E" },
    { SandboxAction::MoveDown, 'Q', L"Move Down", L"Q" }
}};

bool SandboxApp::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    core.LogInfo("SandboxApp: Fresh lab ready.");
    core.LogInfo("SandboxApp: Previous scenario shell archived under apps/sandbox/archive.");

    m_state = SandboxState{};
    ResetCamera();

    core.SetFrameCallback(
        [this](EngineCore& callbackCore)
        {
            UpdateFrame(callbackCore);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void SandboxApp::UpdateFrame(EngineCore& core)
{
    UpdateLogic(core);

    const std::uint32_t red = static_cast<std::uint32_t>(14.0 + (m_state.pulse * 18.0));
    const std::uint32_t green = static_cast<std::uint32_t>(20.0 + (m_state.pulse * 24.0));
    const std::uint32_t blue = static_cast<std::uint32_t>(30.0 + (m_state.pulse * 36.0));
    const std::uint32_t clearColor =
        0xFF000000u |
        (red << 16) |
        (green << 8) |
        blue;

    const FramePrototype frame = BuildLabFrame();
    m_state.objectCount = CountFrameItems(frame);

    core.ClearFrame(clearColor);
    core.SubmitRenderFrame(frame);
    core.SetWindowOverlayText(BuildOverlay(core));
}

void SandboxApp::UpdateLogic(EngineCore& core)
{
    ++m_state.frameCount;
    m_state.totalTime = core.GetTotalTime();
    m_state.pulse = (std::sin(m_state.totalTime * 1.35) + 1.0) * 0.5;

    if (IsActionPressed(core, SandboxAction::ToggleRotation))
    {
        m_state.rotationPaused = !m_state.rotationPaused;
        core.LogInfo(
            std::string("SandboxApp: Rotation ") +
            (m_state.rotationPaused ? "paused" : "resumed"));
    }

    if (IsActionPressed(core, SandboxAction::ResetCamera))
    {
        ResetCamera();
        core.LogInfo("SandboxApp: Camera reset.");
    }

    UpdateCamera(core);
}

void SandboxApp::UpdateCamera(EngineCore& core)
{
    const float moveSpeed = 4.5f * static_cast<float>(core.GetDeltaTime());

    if (IsActionDown(core, SandboxAction::MoveForward))
    {
        m_state.camera.MoveForward(moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveBackward))
    {
        m_state.camera.MoveForward(-moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveLeft))
    {
        m_state.camera.MoveRight(-moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveRight))
    {
        m_state.camera.MoveRight(moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveUp))
    {
        m_state.camera.MoveUp(moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveDown))
    {
        m_state.camera.MoveUp(-moveSpeed);
    }
}

void SandboxApp::ResetCamera()
{
    m_state.camera = CameraPrototype{};
    m_state.camera.position = Vector3(0.0f, 2.0f, -12.0f);
    m_state.camera.LookAt(Vector3(0.0f, 0.75f, 6.0f));
    m_state.camera.SetPerspective(62.0f, 0.1f, 100.0f);
}

FramePrototype SandboxApp::BuildLabFrame() const
{
    ViewPrototype view;
    view.kind = ViewKind::Scene3D;
    view.camera = m_state.camera;

    const float rotationTime = m_state.rotationPaused ? 0.0f : static_cast<float>(m_state.totalTime);
    view.items.reserve(5);

    ItemPrototype planeItem;
    planeItem.kind = ItemKind::InfinitePlane;
    planeItem.infinitePlane.origin = Vector3(0.0f, -2.0f, 0.0f);
    planeItem.infinitePlane.baseColor = 0xFF101820u;
    planeItem.infinitePlane.minorLineColor = 0xFF1E3442u;
    planeItem.infinitePlane.majorLineColor = 0xFF50718Cu;
    planeItem.infinitePlane.minorGridSpacing = 1.0f;
    planeItem.infinitePlane.majorGridSpacing = 4.0f;
    planeItem.infinitePlane.lineThickness = 0.025f;
    planeItem.infinitePlane.extent = 24.0f;
    view.items.push_back(planeItem);

    AddObject(
        view,
        BuiltInMeshKind::Triangle,
        Vector3(0.0f, -0.15f, 7.0f),
        Vector3(2.2f, 2.2f, 2.2f),
        Vector3(rotationTime * 0.15f, rotationTime * 0.40f, 0.0f),
        0xFFE0614Au);
    AddObject(
        view,
        BuiltInMeshKind::Cube,
        Vector3(0.0f, -0.65f, 10.0f),
        Vector3(1.4f, 1.4f, 1.4f),
        Vector3(rotationTime * 0.25f, rotationTime * 0.20f, rotationTime * 0.10f),
        0xFF6FA8DCu);
    AddObject(
        view,
        BuiltInMeshKind::Diamond,
        Vector3(-4.0f, -0.4f, 12.0f),
        Vector3(0.35f, 1.6f, 0.35f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFFB7C8D6u);
    AddObject(
        view,
        BuiltInMeshKind::Diamond,
        Vector3(4.0f, -0.4f, 12.0f),
        Vector3(0.35f, 1.6f, 0.35f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFFB7C8D6u);

    PassPrototype pass;
    pass.kind = PassKind::Scene;
    pass.views.push_back(std::move(view));

    FramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}

std::wstring SandboxApp::BuildOverlay(const EngineCore& core) const
{
    return
        L"SANDBOX LAB\n"
        L"  Frames: " + std::to_wstring(m_state.frameCount) + L"\n"
        L"  Uptime: " + std::to_wstring(m_state.totalTime) + L"s\n"
        L"  Objects: " + std::to_wstring(m_state.objectCount) + L"\n"
        L"  Rotation: " + std::wstring(m_state.rotationPaused ? L"Paused" : L"Live") + L"\n"
        L"  Submitted Items: " + std::to_wstring(core.GetRenderSubmittedItemCount()) + L"\n"
        L"  Worker Count: " + std::to_wstring(core.GetJobWorkerCount()) + L"\n"
        L"  Goal: triangle-first prototype lab\n"
        L"  " + std::wstring(GetBinding(SandboxAction::ToggleRotation).keyLabel) + L": pause/resume rotation\n"
        L"  " + std::wstring(GetBinding(SandboxAction::ResetCamera).keyLabel) + L": reset camera\n"
        L"  " + std::wstring(GetBinding(SandboxAction::MoveForward).keyLabel) + L"/" +
            GetBinding(SandboxAction::MoveLeft).keyLabel + L"/" +
            GetBinding(SandboxAction::MoveBackward).keyLabel + L"/" +
            GetBinding(SandboxAction::MoveRight).keyLabel + L": move\n"
        L"  " + std::wstring(GetBinding(SandboxAction::MoveDown).keyLabel) + L"/" +
            GetBinding(SandboxAction::MoveUp).keyLabel + L": down/up\n"
        L"  Focus: triangle, cube, ground plane, and two depth markers\n"
        L"  Escape: exit\n";
}

bool SandboxApp::IsActionPressed(EngineCore& core, SandboxAction action) const
{
    return core.WasKeyPressed(GetBinding(action).key);
}

bool SandboxApp::IsActionDown(EngineCore& core, SandboxAction action) const
{
    return core.IsKeyDown(GetBinding(action).key);
}

const SandboxApp::InputBinding& SandboxApp::GetBinding(SandboxAction action) const
{
    return kInputBindings[static_cast<std::size_t>(action)];
}

unsigned int SandboxApp::CountFrameItems(const FramePrototype& frame) const
{
    unsigned int itemCount = 0;

    for (const PassPrototype& pass : frame.passes)
    {
        for (const ViewPrototype& view : pass.views)
        {
            itemCount += static_cast<unsigned int>(view.items.size());
        }
    }

    return itemCount;
}

void SandboxApp::AddObject(
    ViewPrototype& view,
    BuiltInMeshKind meshKind,
    const Vector3& translation,
    const Vector3& scale,
    const Vector3& rotation,
    std::uint32_t color) const
{
    ItemPrototype item;
    item.kind = ItemKind::Object3D;
    item.object3D.GetPrimaryMesh().builtInKind = meshKind;
    item.object3D.transform.translation = translation;
    item.object3D.transform.scale = scale;
    item.object3D.transform.rotationRadians = rotation;
    item.object3D.GetPrimaryMesh().material.baseColor = color;
    view.items.push_back(item);
}
