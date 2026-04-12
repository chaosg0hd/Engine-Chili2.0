#include "sandbox_app.hpp"

#include "core/engine_core.hpp"
#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
    constexpr float kCameraMoveSpeed = 7.0f;
    constexpr float kLightMoveSpeed = 4.0f;
    constexpr unsigned int kMinRaycastCount = 1U;
    constexpr unsigned int kMaxRaycastCount = 256U;
    constexpr unsigned char kRestartKey = 'R';
    constexpr unsigned char kAnimateKey = 'T';
    constexpr unsigned char kDecreaseRaysKey = '[';
    constexpr unsigned char kIncreaseRaysKey = ']';

    Vector3 RotateAroundY(const Vector3& vector, float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        return Vector3(
            (vector.x * c) + (vector.z * s),
            vector.y,
            (-vector.x * s) + (vector.z * c));
    }
}

bool SandboxApp::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    core.LogInfo("SandboxApp: Light ray lab ready.");
    ResetLab();

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

    constexpr std::uint32_t clearColor = 0xFF03070Au;
    const FramePrototype frame = BuildLightLabFrame();

    core.ClearFrame(clearColor);
    core.SubmitRenderFrame(frame);
    core.SetWindowOverlayText(BuildOverlay(core));
}

void SandboxApp::UpdateLogic(EngineCore& core)
{
    ++m_state.frameCount;
    m_state.totalTime = core.GetTotalTime();

    const float windowAspectRatio = core.GetWindowAspectRatio();
    if (windowAspectRatio > 0.000001f)
    {
        m_state.aspectRatio = static_cast<double>(windowAspectRatio);
    }

    if (core.WasKeyPressed(kRestartKey))
    {
        ResetLab();
        core.LogInfo("SandboxApp: Light lab reset.");
        return;
    }

    if (core.WasKeyPressed(kAnimateKey))
    {
        m_state.animateLight = !m_state.animateLight;
    }

    if (core.WasKeyPressed(kDecreaseRaysKey))
    {
        m_state.raycastCount = std::max(kMinRaycastCount, m_state.raycastCount / 2U);
    }

    if (core.WasKeyPressed(kIncreaseRaysKey))
    {
        m_state.raycastCount = std::min(kMaxRaycastCount, m_state.raycastCount * 2U);
    }

    UpdateCameraMovement(core);
    UpdateLightControls(core);
}

void SandboxApp::UpdateCameraMovement(EngineCore& core)
{
    const float moveDistance = kCameraMoveSpeed * static_cast<float>(core.GetDeltaTime());

    if (core.IsKeyDown('W'))
    {
        m_state.camera.MoveForward(moveDistance);
    }

    if (core.IsKeyDown('S'))
    {
        m_state.camera.MoveForward(-moveDistance);
    }

    if (core.IsKeyDown('A'))
    {
        m_state.camera.MoveRight(-moveDistance);
    }

    if (core.IsKeyDown('D'))
    {
        m_state.camera.MoveRight(moveDistance);
    }

    if (core.IsKeyDown('Q'))
    {
        m_state.camera.MoveUp(-moveDistance);
    }

    if (core.IsKeyDown('E'))
    {
        m_state.camera.MoveUp(moveDistance);
    }
}

void SandboxApp::UpdateLightControls(EngineCore& core)
{
    const float moveDistance = kLightMoveSpeed * static_cast<float>(core.GetDeltaTime());

    if (core.IsKeyDown('J'))
    {
        m_state.lightOrigin.x -= moveDistance;
    }

    if (core.IsKeyDown('L'))
    {
        m_state.lightOrigin.x += moveDistance;
    }

    if (core.IsKeyDown('I'))
    {
        m_state.lightOrigin.z += moveDistance;
    }

    if (core.IsKeyDown('K'))
    {
        m_state.lightOrigin.z -= moveDistance;
    }

    if (core.IsKeyDown('U'))
    {
        m_state.lightOrigin.y += moveDistance;
    }

    if (core.IsKeyDown('O'))
    {
        m_state.lightOrigin.y -= moveDistance;
    }
}

void SandboxApp::ResetLab()
{
    m_state = SandboxState{};
    m_state.camera = CameraPrototype{};
    m_state.camera.position = Vector3(0.0f, 2.2f, -10.0f);
    m_state.camera.LookAt(Vector3(0.0f, 0.8f, 5.0f));
    m_state.camera.SetPerspective(58.0f, 0.1f, 120.0f);
}

FramePrototype SandboxApp::BuildLightLabFrame() const
{
    ViewPrototype view;
    view.kind = ViewKind::Scene3D;
    view.camera = m_state.camera;
    view.lights.reserve(1);
    view.items.reserve(static_cast<std::size_t>(m_state.raycastCount) + 8U);

    Vector3 lightDirection = Normalize(m_state.lightDirection);
    if (m_state.animateLight)
    {
        const float yaw = std::sin(static_cast<float>(m_state.totalTime) * 0.55f) * 0.55f;
        const float bob = std::sin(static_cast<float>(m_state.totalTime) * 0.85f) * 0.18f;
        lightDirection = Normalize(RotateAroundY(Vector3(0.0f, -0.08f + bob, 1.0f), yaw));
    }

    LightPrototype labLight;
    labLight.SetRay(m_state.lightOrigin, lightDirection);
    labLight.color = ColorPrototype::FromBytes(180, 225, 255);
    labLight.intensity = 1.0f;
    labLight.raycastCount = m_state.raycastCount;
    view.lights.push_back(labLight);

    AddCube(view, Vector3(0.0f, -1.2f, 5.0f), Vector3(9.0f, 0.08f, 12.0f), Vector3(0.0f, 0.0f, 0.0f), 0xFF101820u);
    AddCube(view, Vector3(-3.0f, 0.25f, 6.2f), Vector3(0.9f, 2.2f, 0.9f), Vector3(0.0f, 0.2f, 0.0f), 0xFF1F6F8Bu);
    AddCube(view, Vector3(2.7f, 0.05f, 5.2f), Vector3(1.2f, 1.8f, 1.2f), Vector3(0.0f, -0.45f, 0.0f), 0xFF8B2E3Eu);
    AddCube(view, Vector3(0.0f, 0.55f, 8.3f), Vector3(4.2f, 0.2f, 0.6f), Vector3(0.0f, 0.0f, 0.0f), 0xFFE0D7A4u);
    AddCube(view, m_state.lightOrigin, Vector3(0.28f, 0.28f, 0.28f), Vector3(0.0f, 0.0f, 0.0f), 0xFFFFF2A6u);

    const unsigned int visibleRayCount = std::min(m_state.raycastCount, 96U);
    for (unsigned int index = 0; index < visibleRayCount; ++index)
    {
        const float normalizedIndex =
            visibleRayCount > 1U
                ? (static_cast<float>(index) / static_cast<float>(visibleRayCount - 1U))
                : 0.5f;
        const float fanAngle = (normalizedIndex - 0.5f) * 1.1f;
        const float ripple = std::sin((static_cast<float>(index) * 2.17f) + static_cast<float>(m_state.totalTime)) * 0.045f;
        const Vector3 rayDirection = Normalize(RotateAroundY(lightDirection, fanAngle) + Vector3(0.0f, ripple, 0.0f));
        const float length = 4.0f + (normalizedIndex * 5.5f);
        const std::uint32_t rayColor = (index % 3U == 0U) ? 0xFF6EE7F9u : 0xFFB7F7FFu;
        AddRayMarker(view, m_state.lightOrigin, rayDirection, length, 0.025f, rayColor);
    }

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
        L"LIGHT RAY LAB\n"
        L"  Submitted Items: " + std::to_wstring(core.GetRenderSubmittedItemCount()) + L"\n"
        L"  Raycast Count: " + std::to_wstring(m_state.raycastCount) + L"\n"
        L"  Visualized Rays: " + std::to_wstring(std::min(m_state.raycastCount, 96U)) + L"\n"
        L"  Light Animation: " + std::wstring(m_state.animateLight ? L"On" : L"Off") + L"\n"
        L"  WASD/QE: move camera\n"
        L"  IJKL/UO: move light source\n"
        L"  [/]: halve/double ray count\n"
        L"  T: toggle light sweep\n"
        L"  R: reset\n"
        L"  Escape: exit\n";
}

void SandboxApp::AddCube(
    ViewPrototype& view,
    const Vector3& position,
    const Vector3& scale,
    const Vector3& rotation,
    std::uint32_t color) const
{
    ItemPrototype item;
    item.kind = ItemKind::Object3D;
    item.object3D.GetPrimaryMesh().builtInKind = BuiltInMeshKind::Cube;
    item.object3D.transform.translation = position;
    item.object3D.transform.scale = scale;
    item.object3D.transform.rotationRadians = rotation;
    item.object3D.GetPrimaryMesh().material.baseColor = ColorPrototype::FromArgb(color);
    view.items.push_back(item);
}

void SandboxApp::AddRayMarker(
    ViewPrototype& view,
    const Vector3& origin,
    const Vector3& direction,
    float length,
    float thickness,
    std::uint32_t color) const
{
    const Vector3 normalizedDirection = Normalize(direction);
    const Vector3 midpoint = origin + (normalizedDirection * (length * 0.5f));
    const float yaw = std::atan2(normalizedDirection.x, normalizedDirection.z);
    AddCube(
        view,
        midpoint,
        Vector3(thickness, thickness, length),
        Vector3(0.0f, yaw, 0.0f),
        color);
}
