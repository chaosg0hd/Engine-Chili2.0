#include "scenario_shell_sandbox_app.hpp"

#include "core/engine_core.hpp"
#include "prototypes/entity/object/mesh.hpp"
#include "prototypes/entity/scene/camera.hpp"
#include "prototypes/entity/scene/infinite_plane.hpp"
#include "prototypes/math/math.hpp"
#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"
#include "prototypes/presentation/view.hpp"

#include <cmath>
#include <sstream>
#include <string>

const std::array<SandboxApp::InputBinding, static_cast<std::size_t>(SandboxApp::SandboxAction::Count)>
    SandboxApp::kInputBindings =
{{
    { SandboxAction::ToggleRotation, 9, L"Toggle Rotation", L"Tab" },
    { SandboxAction::ToggleCameraOrbit, 32, L"Toggle Camera Orbit", L"Space" },
    { SandboxAction::SelectVisibilityRoom, '1', L"Select Visibility Room", L"1" },
    { SandboxAction::SelectPrototypeGrid, '2', L"Select Prototype Grid", L"2" },
    { SandboxAction::SelectCameraControlLab, '3', L"Select Camera Control Lab", L"3" },
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

    core.LogInfo(
        std::string("SandboxApp: 3D visibility sandbox ready | file logging = ") +
        (core.IsFileLoggingAvailable() ? "true" : "false") +
        " | log path = " +
        core.GetLogFilePath()
    );

    core.LogInfo("SandboxApp: Active harness = direct 3D visibility demo.");
    core.LogInfo("SandboxApp: Active shell = scenario-driven prototype sandbox.");
    core.LogInfo("SandboxApp: Legacy harnesses remain under apps/sandbox/archive.");

    m_state = SandboxState{};
    m_state.cameraControlCamera.position = Vector3(0.0f, 2.2f, -12.5f);
    m_state.cameraControlCamera.LookAt(Vector3(0.0f, 0.8f, 7.0f));
    m_state.cameraControlCamera.SetPerspective(62.0f, 0.1f, 100.0f);
    InitializeSandboxLog(core);

    core.SetFrameCallback(
        [this](EngineCore& callbackCore)
        {
            UpdateFrame(callbackCore);
        });

    const bool success = core.Run();
    FlushSandboxLog(core, true);
    core.Shutdown();
    return success;
}

void SandboxApp::UpdateFrame(EngineCore& core)
{
    UpdateLogic(core);

    const std::uint32_t red = static_cast<std::uint32_t>(20.0 + (m_state.pulse * 22.0));
    const std::uint32_t green = static_cast<std::uint32_t>(26.0 + (m_state.pulse * 34.0));
    const std::uint32_t blue = static_cast<std::uint32_t>(38.0 + (m_state.pulse * 54.0));
    const std::uint32_t clearColor =
        0xFF000000u |
        (red << 16) |
        (green << 8) |
        blue;
    const FramePrototype frame = BuildActiveFrame();
    m_state.objectCount = CountFrameItems(frame);

    core.ClearFrame(clearColor);
    core.SubmitRenderFrame(frame);
    FlushSandboxLog(core, false);
    core.SetWindowOverlayText(BuildPresentationOverlay(core));
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
            (m_state.rotationPaused ? "paused" : "resumed")
        );

        AppendSandboxLogLine(
            std::string("rotation_paused=") +
            (m_state.rotationPaused ? "true" : "false"));
    }

    if (IsActionPressed(core, SandboxAction::ToggleCameraOrbit))
    {
        m_state.cameraOrbitEnabled = !m_state.cameraOrbitEnabled;
        core.LogInfo(
            std::string("SandboxApp: Camera orbit ") +
            (m_state.cameraOrbitEnabled ? "enabled" : "disabled")
        );

        AppendSandboxLogLine(
            std::string("camera_orbit_enabled=") +
            (m_state.cameraOrbitEnabled ? "true" : "false"));
    }

    if (IsActionPressed(core, SandboxAction::SelectVisibilityRoom))
    {
        TrySelectScene(core, SandboxScene::VisibilityRoom, "hotkey_1");
    }

    if (IsActionPressed(core, SandboxAction::SelectPrototypeGrid))
    {
        TrySelectScene(core, SandboxScene::PrototypeGrid, "hotkey_2");
    }

    if (IsActionPressed(core, SandboxAction::SelectCameraControlLab))
    {
        TrySelectScene(core, SandboxScene::CameraControlLab, "hotkey_3");
    }

    if (m_state.activeScene == SandboxScene::CameraControlLab)
    {
        UpdateCameraControlLab(core);
    }
}

void SandboxApp::UpdateCameraControlLab(EngineCore& core)
{
    const float moveSpeed = 4.5f * static_cast<float>(core.GetDeltaTime());

    if (IsActionDown(core, SandboxAction::MoveForward))
    {
        m_state.cameraControlCamera.MoveForward(moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveBackward))
    {
        m_state.cameraControlCamera.MoveForward(-moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveLeft))
    {
        m_state.cameraControlCamera.MoveRight(-moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveRight))
    {
        m_state.cameraControlCamera.MoveRight(moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveUp))
    {
        m_state.cameraControlCamera.MoveUp(moveSpeed);
    }

    if (IsActionDown(core, SandboxAction::MoveDown))
    {
        m_state.cameraControlCamera.MoveUp(-moveSpeed);
    }
}

FramePrototype SandboxApp::BuildActiveFrame() const
{
    switch (m_state.activeScene)
    {
    case SandboxScene::PrototypeGrid:
        return BuildPrototypeGridFrame();
    case SandboxScene::CameraControlLab:
        return BuildCameraControlLabFrame();
    case SandboxScene::VisibilityRoom:
    default:
        return BuildVisibilityRoomFrame();
    }
}

FramePrototype SandboxApp::BuildVisibilityRoomFrame() const
{
    ViewPrototype view = BuildDefaultSceneView();
    const float rotationTime = m_state.rotationPaused ? 0.0f : static_cast<float>(m_state.totalTime);
    view.items.reserve(8);

    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(0.0f, -2.25f, 3.6f),
        Vector3(7.2f, 0.12f, 7.2f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF2E4057u);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(0.0f, 3.15f, 3.6f),
        Vector3(7.2f, 0.12f, 7.2f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF3A506Bu);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(-3.6f, 0.45f, 3.6f),
        Vector3(0.12f, 5.4f, 7.2f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF3F5D5Bu);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(3.6f, 0.45f, 3.6f),
        Vector3(0.12f, 5.4f, 7.2f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF3F5D5Bu);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(0.0f, 0.45f, 7.2f),
        Vector3(7.2f, 5.4f, 0.12f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF34495Eu);
    AddObject(
        view,
        BuiltInMeshKind::Octahedron,
        Vector3(0.0f, -0.45f, 3.2f),
        Vector3(1.35f, 1.35f, 1.35f),
        Vector3(rotationTime * 0.55f, rotationTime * 0.35f, rotationTime * 0.25f),
        0xFFD9C27Au);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(0.0f, 2.72f, 3.2f),
        Vector3(0.55f, 0.55f, 0.55f),
        Vector3(0.0f, rotationTime * 0.2f, 0.0f),
        0xFFFFF3B0u);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(0.0f, 2.45f, 3.2f),
        Vector3(0.18f, 0.8f, 0.18f),
        Vector3(0.0f, rotationTime * 0.2f, 0.0f),
        0xFF8D99AEu);

    PassPrototype pass;
    pass.kind = PassKind::Scene;
    pass.views.push_back(std::move(view));

    FramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}

FramePrototype SandboxApp::BuildPrototypeGridFrame() const
{
    ViewPrototype view = BuildDefaultSceneView();
    const float rotationTime = m_state.rotationPaused ? 0.0f : static_cast<float>(m_state.totalTime);
    view.camera.target = Vector3(0.0f, 0.25f, 5.4f);
    view.items.reserve(10);

    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(0.0f, -1.95f, 5.4f),
        Vector3(10.0f, 0.08f, 10.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF243447u);

    const BuiltInMeshKind meshes[] =
    {
        BuiltInMeshKind::Triangle,
        BuiltInMeshKind::Diamond,
        BuiltInMeshKind::Quad,
        BuiltInMeshKind::Cube,
        BuiltInMeshKind::Octahedron
    };

    const std::uint32_t colors[] =
    {
        0xFFE76F51u,
        0xFFF4A261u,
        0xFFE9C46Au,
        0xFF2A9D8Fu,
        0xFF4D908Eu
    };

    for (int index = 0; index < 5; ++index)
    {
        const float x = static_cast<float>(index - 2) * 1.85f;
        AddObject(
            view,
            meshes[index],
            Vector3(x, -0.2f, 4.6f),
            Vector3(0.9f, 0.9f, 0.9f),
            Vector3(rotationTime * 0.35f, rotationTime * (0.20f + (0.08f * static_cast<float>(index))), 0.0f),
            colors[index]);

        AddObject(
            view,
            meshes[index],
            Vector3(x, 1.55f, 6.4f),
            Vector3(0.65f, 0.65f, 0.65f),
            Vector3(0.0f, -rotationTime * (0.18f + (0.05f * static_cast<float>(index))), 0.0f),
            colors[4 - index]);
    }

    PassPrototype pass;
    pass.kind = PassKind::Scene;
    pass.views.push_back(std::move(view));

    FramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}

FramePrototype SandboxApp::BuildCameraControlLabFrame() const
{
    ViewPrototype view;
    view.kind = ViewKind::Scene3D;
    view.camera = m_state.cameraControlCamera;
    const float rotationTime = m_state.rotationPaused ? 0.0f : static_cast<float>(m_state.totalTime);
    view.items.reserve(11);

    ItemPrototype planeItem;
    planeItem.kind = ItemKind::InfinitePlane;
    planeItem.infinitePlane.origin = Vector3(0.0f, -2.0f, 0.0f);
    planeItem.infinitePlane.baseColor = 0xFF16202Bu;
    planeItem.infinitePlane.minorLineColor = 0xFF223140u;
    planeItem.infinitePlane.majorLineColor = 0xFF3F5F7Du;
    planeItem.infinitePlane.minorGridSpacing = 1.0f;
    planeItem.infinitePlane.majorGridSpacing = 5.0f;
    planeItem.infinitePlane.lineThickness = 0.03f;
    planeItem.infinitePlane.extent = 20.0f;
    view.items.push_back(planeItem);

    AddObject(
        view,
        BuiltInMeshKind::Cube,
        Vector3(0.0f, -0.8f, 3.0f),
        Vector3(1.1f, 1.1f, 1.1f),
        Vector3(rotationTime * 0.20f, rotationTime * 0.35f, 0.0f),
        0xFFE76F51u);
    AddObject(
        view,
        BuiltInMeshKind::Cube,
        Vector3(-4.0f, -0.8f, 7.0f),
        Vector3(1.0f, 1.0f, 1.0f),
        Vector3(0.0f, rotationTime * 0.15f, 0.0f),
        0xFF4D908Eu);
    AddObject(
        view,
        BuiltInMeshKind::Cube,
        Vector3(4.0f, -0.8f, 7.0f),
        Vector3(1.0f, 1.0f, 1.0f),
        Vector3(0.0f, -rotationTime * 0.15f, 0.0f),
        0xFF577590u);
    AddObject(
        view,
        BuiltInMeshKind::Octahedron,
        Vector3(0.0f, 0.9f, 11.0f),
        Vector3(0.9f, 0.9f, 0.9f),
        Vector3(rotationTime * 0.30f, rotationTime * 0.20f, rotationTime * 0.10f),
        0xFFF4A261u);
    AddObject(
        view,
        BuiltInMeshKind::Triangle,
        Vector3(0.0f, 2.1f, 15.0f),
        Vector3(0.95f, 0.95f, 0.95f),
        Vector3(0.0f, rotationTime * 0.25f, 0.0f),
        0xFFF94144u);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(0.0f, 4.8f, 7.0f),
        Vector3(16.0f, 0.05f, 22.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF253445u);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(-8.0f, 1.4f, 7.0f),
        Vector3(0.05f, 6.8f, 22.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF31465Au);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(8.0f, 1.4f, 7.0f),
        Vector3(0.05f, 6.8f, 22.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF31465Au);
    AddObject(
        view,
        BuiltInMeshKind::Quad,
        Vector3(0.0f, 1.4f, 18.0f),
        Vector3(16.0f, 6.8f, 0.05f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF2A3C4Eu);
    AddObject(
        view,
        BuiltInMeshKind::Diamond,
        Vector3(0.0f, -1.2f, 7.0f),
        Vector3(0.55f, 0.55f, 0.55f),
        Vector3(0.0f, rotationTime * 0.40f, 0.0f),
        0xFFFFFF99u);

    PassPrototype pass;
    pass.kind = PassKind::Scene;
    pass.views.push_back(std::move(view));

    FramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}

std::wstring SandboxApp::BuildPresentationOverlay(const EngineCore& core) const
{
    return
        L"SANDBOX PROTOTYPE SHELL\n"
        L"  Scene: " + GetActiveSceneNameWide() + L"\n"
        L"  Frames: " + std::to_wstring(m_state.frameCount) + L"\n"
        L"  Uptime: " + std::to_wstring(m_state.totalTime) + L"s\n"
        L"  Objects: " + std::to_wstring(m_state.objectCount) + L"\n"
        L"  Rotation: " + std::wstring(m_state.rotationPaused ? L"Paused" : L"Live") + L"\n"
        L"  Camera Orbit: " + std::wstring(m_state.cameraOrbitEnabled ? L"Live" : L"Locked") + L"\n"
        L"  Submitted Items: " + std::to_wstring(core.GetRenderSubmittedItemCount()) + L"\n"
        L"  Worker Count: " + std::to_wstring(core.GetJobWorkerCount()) + L"\n"
        L"  Shell Goal: fast prototype scene swapping\n"
        L"  Log: sandbox_3d_visibility.txt\n"
        L"  " + std::wstring(GetBinding(SandboxAction::SelectVisibilityRoom).keyLabel) + L"/" +
            GetBinding(SandboxAction::SelectPrototypeGrid).keyLabel + L"/" +
            GetBinding(SandboxAction::SelectCameraControlLab).keyLabel + L": switch scenes\n"
        L"  " + std::wstring(GetBinding(SandboxAction::ToggleRotation).keyLabel) + L": pause/resume rotation\n"
        L"  " + std::wstring(GetBinding(SandboxAction::ToggleCameraOrbit).keyLabel) + L": toggle scripted orbit (other scenes)\n"
        L"  " + std::wstring(GetBinding(SandboxAction::MoveForward).keyLabel) + L"/" +
            GetBinding(SandboxAction::MoveLeft).keyLabel + L"/" +
            GetBinding(SandboxAction::MoveBackward).keyLabel + L"/" +
            GetBinding(SandboxAction::MoveRight).keyLabel + L": move camera lab\n"
        L"  " + std::wstring(GetBinding(SandboxAction::MoveDown).keyLabel) + L"/" +
            GetBinding(SandboxAction::MoveUp).keyLabel + L": down/up\n"
        L"  Escape: exit\n";
}

void SandboxApp::InitializeSandboxLog(EngineCore& core)
{
    m_sandboxLogBuffer.clear();
    m_nextSandboxLogFlushTime = 0.0;
    core.CreateDirectory("logs");

    std::ostringstream header;
    header
        << "Sandbox Prototype Shell Log\n"
        << "log_path=" << m_sandboxLogPath << '\n'
        << "engine_log_path=" << core.GetLogFilePath() << '\n'
        << "initial_scene=" << GetSceneName(m_state.activeScene) << '\n';
    m_sandboxLogBuffer = header.str();
    core.WriteTextFile(m_sandboxLogPath, m_sandboxLogBuffer);
}

void SandboxApp::AppendSandboxLogLine(const std::string& line)
{
    m_sandboxLogBuffer += line;
    m_sandboxLogBuffer += '\n';
}

void SandboxApp::FlushSandboxLog(EngineCore& core, bool force)
{
    if (!force && core.GetTotalTime() < m_nextSandboxLogFlushTime)
    {
        return;
    }

    std::ostringstream snapshot;
    snapshot
        << "snapshot"
        << " total_time=" << m_state.totalTime
        << " frames=" << m_state.frameCount
        << " scene=" << GetSceneName(m_state.activeScene)
        << " objects=" << m_state.objectCount
        << " rotation_paused=" << (m_state.rotationPaused ? "true" : "false")
        << " camera_orbit=" << (m_state.cameraOrbitEnabled ? "true" : "false")
        << " submitted_items=" << core.GetRenderSubmittedItemCount();
    AppendSandboxLogLine(snapshot.str());

    core.WriteTextFile(m_sandboxLogPath, m_sandboxLogBuffer);
    m_nextSandboxLogFlushTime = core.GetTotalTime() + 0.5;
}

bool SandboxApp::TrySelectScene(EngineCore& core, SandboxScene scene, const char* reason)
{
    if (m_state.activeScene == scene)
    {
        return false;
    }

    m_state.activeScene = scene;
    core.LogInfo(
        std::string("SandboxApp: Active scene changed to ") +
        GetSceneName(scene));
    AppendSandboxLogLine(
        std::string("scene_changed=") +
        GetSceneName(scene) +
        " reason=" +
        (reason ? reason : "unknown"));
    return true;
}

const char* SandboxApp::GetSceneName(SandboxScene scene) const
{
    switch (scene)
    {
    case SandboxScene::PrototypeGrid:
        return "PrototypeGrid";
    case SandboxScene::CameraControlLab:
        return "CameraControlLab";
    case SandboxScene::VisibilityRoom:
    default:
        return "VisibilityRoom";
    }
}

std::wstring SandboxApp::GetActiveSceneNameWide() const
{
    const char* sceneName = GetSceneName(m_state.activeScene);
    return std::wstring(sceneName, sceneName + std::char_traits<char>::length(sceneName));
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

ViewPrototype SandboxApp::BuildDefaultSceneView() const
{
    ViewPrototype view;
    view.kind = ViewKind::Scene3D;
    view.camera = CameraPrototype{};

    const float orbitAngle = m_state.cameraOrbitEnabled ? static_cast<float>(m_state.totalTime * 0.35) : 0.0f;
    view.camera.position = Vector3(
        std::sin(orbitAngle) * 1.25f,
        0.55f + (std::sin(orbitAngle * 0.45f) * 0.12f),
        -10.2f + (std::cos(orbitAngle) * 0.4f));
    view.camera.target = Vector3(0.0f, 0.35f, 3.4f);
    view.camera.fovDegrees = 62.0f;
    view.camera.nearPlane = 0.1f;
    view.camera.farPlane = 100.0f;
    return view;
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
    item.object3D.mesh.builtInKind = meshKind;
    item.object3D.transform.translation = translation;
    item.object3D.transform.scale = scale;
    item.object3D.transform.rotationRadians = rotation;
    item.object3D.material.baseColor = color;
    view.items.push_back(item);
}
