#include "sandbox_app.hpp"

#include "core/engine_core.hpp"
#include "prototypes/entity/camera.hpp"
#include "prototypes/entity/mesh.hpp"
#include "prototypes/math/math.hpp"
#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"
#include "prototypes/presentation/view.hpp"

#include <cmath>
#include <sstream>
#include <string>

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
    core.LogInfo("SandboxApp: Legacy harnesses remain under apps/sandbox/archive.");

    m_state = SandboxState{};
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

    core.ClearFrame(clearColor);
    core.SubmitRenderFrame(BuildDemoFrame());
    FlushSandboxLog(core, false);
    core.SetWindowOverlayText(BuildPresentationOverlay(core));
}

void SandboxApp::UpdateLogic(EngineCore& core)
{
    ++m_state.frameCount;
    m_state.totalTime = core.GetTotalTime();
    m_state.pulse = (std::sin(m_state.totalTime * 1.35) + 1.0) * 0.5;
    m_state.objectCount = 8U;

    constexpr unsigned char kTabKey = 9;
    if (core.WasKeyPressed(kTabKey))
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

    constexpr unsigned char kSpaceKey = 32;
    if (core.WasKeyPressed(kSpaceKey))
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
}

FramePrototype SandboxApp::BuildDemoFrame() const
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
    view.items.reserve(m_state.objectCount);

    const float rotationTime = m_state.rotationPaused ? 0.0f : static_cast<float>(m_state.totalTime);
    auto pushObject =
        [&view](
            BuiltInMeshKind meshKind,
            const Vector3& translation,
            const Vector3& scale,
            const Vector3& rotation,
            std::uint32_t color)
        {
            ItemPrototype item;
            item.kind = ItemKind::Object3D;
            item.object3D.mesh.builtInKind = meshKind;
            item.object3D.transform.translation = translation;
            item.object3D.transform.scale = scale;
            item.object3D.transform.rotationRadians = rotation;
            item.object3D.material.baseColor = color;
            view.items.push_back(item);
        };

    pushObject(
        BuiltInMeshKind::Quad,
        Vector3(0.0f, -2.25f, 3.6f),
        Vector3(7.2f, 0.12f, 7.2f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF2E4057u);

    pushObject(
        BuiltInMeshKind::Quad,
        Vector3(0.0f, 3.15f, 3.6f),
        Vector3(7.2f, 0.12f, 7.2f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF3A506Bu);

    pushObject(
        BuiltInMeshKind::Quad,
        Vector3(-3.6f, 0.45f, 3.6f),
        Vector3(0.12f, 5.4f, 7.2f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF3F5D5Bu);

    pushObject(
        BuiltInMeshKind::Quad,
        Vector3(3.6f, 0.45f, 3.6f),
        Vector3(0.12f, 5.4f, 7.2f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF3F5D5Bu);

    pushObject(
        BuiltInMeshKind::Quad,
        Vector3(0.0f, 0.45f, 7.2f),
        Vector3(7.2f, 5.4f, 0.12f),
        Vector3(0.0f, 0.0f, 0.0f),
        0xFF34495Eu);

    pushObject(
        BuiltInMeshKind::Octahedron,
        Vector3(0.0f, -0.45f, 3.2f),
        Vector3(1.35f, 1.35f, 1.35f),
        Vector3(rotationTime * 0.55f, rotationTime * 0.35f, rotationTime * 0.25f),
        0xFFD9C27Au);

    pushObject(
        BuiltInMeshKind::Quad,
        Vector3(0.0f, 2.72f, 3.2f),
        Vector3(0.55f, 0.55f, 0.55f),
        Vector3(0.0f, rotationTime * 0.2f, 0.0f),
        0xFFFFF3B0u);

    pushObject(
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

std::wstring SandboxApp::BuildPresentationOverlay(const EngineCore& core) const
{
    return
        L"SANDBOX WALLED ROOM TEST\n"
        L"  Frames: " + std::to_wstring(m_state.frameCount) + L"\n"
        L"  Uptime: " + std::to_wstring(m_state.totalTime) + L"s\n"
        L"  Objects: " + std::to_wstring(m_state.objectCount) + L"\n"
        L"  Rotation: " + std::wstring(m_state.rotationPaused ? L"Paused" : L"Live") + L"\n"
        L"  Camera Orbit: " + std::wstring(m_state.cameraOrbitEnabled ? L"Live" : L"Locked") + L"\n"
        L"  Submitted Items: " + std::to_wstring(core.GetRenderSubmittedItemCount()) + L"\n"
        L"  Worker Count: " + std::to_wstring(core.GetJobWorkerCount()) + L"\n"
        L"  Expect: room + low-poly sphere + ceiling light marker\n"
        L"  Log: sandbox_3d_visibility.txt\n"
        L"  Tab: pause/resume rotation\n"
        L"  Space: toggle camera orbit\n"
        L"  Escape: exit\n";
}

void SandboxApp::InitializeSandboxLog(EngineCore& core)
{
    m_sandboxLogBuffer.clear();
    m_nextSandboxLogFlushTime = 0.0;
    core.CreateDirectory("logs");

    std::ostringstream header;
    header
        << "Sandbox 3D Visibility Log\n"
        << "log_path=" << m_sandboxLogPath << '\n'
        << "engine_log_path=" << core.GetLogFilePath() << '\n';
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
        << " objects=" << m_state.objectCount
        << " rotation_paused=" << (m_state.rotationPaused ? "true" : "false")
        << " camera_orbit=" << (m_state.cameraOrbitEnabled ? "true" : "false")
        << " submitted_items=" << core.GetRenderSubmittedItemCount();
    AppendSandboxLogLine(snapshot.str());

    core.WriteTextFile(m_sandboxLogPath, m_sandboxLogBuffer);
    m_nextSandboxLogFlushTime = core.GetTotalTime() + 0.5;
}
