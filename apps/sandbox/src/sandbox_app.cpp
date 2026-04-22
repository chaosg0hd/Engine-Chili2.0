#include "sandbox_app.hpp"

#include "app/app_capabilities.hpp"
#include "core/engine_core.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace
{
CameraPrototype BuildLightingLabDefaultCamera()
{
    CameraPrototype camera;
    camera.position = Vector3(0.0f, 1.7f, 0.8f);
    camera.target = Vector3(0.0f, 0.6f, 4.5f);
    camera.fovDegrees = 52.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 200.0f;
    camera.exposure = 1.0f;
    return camera;
}

SceneLightPrototype BuildLightingLabDefaultLight()
{
    SceneLightPrototype light;
    light.kind = SceneLightKind::Point;
    light.position = Vector3(0.0f, 2.4f, 4.5f);
    light.color = ColorPrototype::FromArgb(0xFFFFF1D0u);
    light.intensity = 3.4f;
    light.range = 9.0f;
    light.enabled = true;
    return light;
}
}

bool SandboxApp::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    ConfigureSandbox();
    AppCapabilities& capabilities = core.GetAppCapabilities();
    capabilities.logging->Info("SandboxApp: DX lighting sandbox ready.");

    m_state = SandboxState{};
    ResetCamera();
    if (!InitializeMaterialPrototypes(capabilities))
    {
        capabilities.logging->Error("SandboxApp: required lighting lab material prototypes are unavailable.");
        core.Shutdown();
        return false;
    }

    core.SetFrameCallback(
        [this](AppCapabilities& callbackCapabilities)
        {
            UpdateFrame(callbackCapabilities);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void SandboxApp::ConfigureSandbox()
{
    m_scene = SandboxPresetScenePrototype::LightingLab;
}

bool SandboxApp::InitializeMaterialPrototypes(const AppCapabilities& capabilities)
{
    if (!capabilities.prototypes)
    {
        return false;
    }

    const MaterialPrototype* floorMaterial =
        capabilities.prototypes->GetMaterialPrototype("lighting_lab/stucco_floor");
    const MaterialPrototype* roomMaterial =
        capabilities.prototypes->GetMaterialPrototype("lighting_lab/stucco_room");
    const MaterialPrototype* cubeMaterial =
        capabilities.prototypes->GetMaterialPrototype("lighting_lab/stucco_cube");
    const MaterialPrototype* emitterMaterial =
        capabilities.prototypes->GetMaterialPrototype("lighting_lab/emitter");

    if (!floorMaterial || !roomMaterial || !cubeMaterial || !emitterMaterial)
    {
        return false;
    }

    m_floorMaterial = *floorMaterial;
    m_roomMaterial = *roomMaterial;
    m_cubeMaterial = *cubeMaterial;
    m_emitterMaterial = *emitterMaterial;
    return true;
}

void SandboxApp::ResetCamera()
{
    m_state.camera = BuildLightingLabDefaultCamera();
    m_state.primarySceneLight = BuildLightingLabDefaultLight();
}

void SandboxApp::UpdateFrame(AppCapabilities& capabilities)
{
    UpdateLogic(capabilities);

    capabilities.rendering->ClearFrame(0xFF0A0D14u);
    capabilities.rendering->SubmitFrame(BuildFrame());
    capabilities.window->SetOverlayText(BuildOverlayText(capabilities));
}

void SandboxApp::UpdateLogic(AppCapabilities& capabilities)
{
    const float deltaTime = static_cast<float>(capabilities.window->GetDeltaTime());

    if (capabilities.window->WasKeyPressed(' '))
    {
        m_state.rotationPaused = !m_state.rotationPaused;
        capabilities.logging->Info(m_state.rotationPaused
            ? "[DX-LIGHTING] Animation paused."
            : "[DX-LIGHTING] Animation resumed.");
    }

    if (capabilities.window->WasKeyPressed('O'))
    {
        m_state.orbitEnabled = !m_state.orbitEnabled;
        capabilities.logging->Info(m_state.orbitEnabled
            ? "[DX-LIGHTING] Camera orbit enabled."
            : "[DX-LIGHTING] Camera orbit disabled.");
    }

    if (capabilities.window->WasKeyPressed('R'))
    {
        m_state = SandboxState{};
        ResetCamera();
        capabilities.logging->Info("[DX-LIGHTING] Sandbox state reset.");
    }

    if (capabilities.window->WasKeyPressed(VK_OEM_MINUS))
    {
        m_state.camera.exposure = std::max(0.1f, m_state.camera.exposure - 0.1f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera exposure set to " << m_state.camera.exposure;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed(VK_OEM_PLUS))
    {
        m_state.camera.exposure = std::min(4.0f, m_state.camera.exposure + 0.1f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Camera exposure set to " << m_state.camera.exposure;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('J'))
    {
        m_state.primarySceneLight.intensity = std::max(0.1f, m_state.primarySceneLight.intensity - 0.5f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light intensity set to " << m_state.primarySceneLight.intensity;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('K'))
    {
        m_state.primarySceneLight.intensity = std::min(20.0f, m_state.primarySceneLight.intensity + 0.5f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light intensity set to " << m_state.primarySceneLight.intensity;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('N'))
    {
        m_state.primarySceneLight.range = std::max(2.0f, m_state.primarySceneLight.range - 1.0f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light range set to " << m_state.primarySceneLight.range;
        capabilities.logging->Info(message.str());
    }

    if (capabilities.window->WasKeyPressed('M'))
    {
        m_state.primarySceneLight.range = std::min(40.0f, m_state.primarySceneLight.range + 1.0f);
        std::ostringstream message;
        message << "[DX-LIGHTING] Primary light range set to " << m_state.primarySceneLight.range;
        capabilities.logging->Info(message.str());
    }

    if (!m_state.orbitEnabled)
    {
        const float moveSpeed = capabilities.window->IsKeyDown(VK_SHIFT) ? 9.0f : 4.5f;
        const float orbitSpeed = 1.6f;
        const float zoomSpeed = 8.0f;
        const float moveDistance = moveSpeed * deltaTime;
        const float orbitStep = orbitSpeed * deltaTime;

        if (capabilities.window->IsKeyDown('W'))
        {
            m_state.camera.MoveForward(moveDistance);
        }
        if (capabilities.window->IsKeyDown('S'))
        {
            m_state.camera.MoveForward(-moveDistance);
        }
        if (capabilities.window->IsKeyDown('A'))
        {
            m_state.camera.MoveRight(-moveDistance);
        }
        if (capabilities.window->IsKeyDown('D'))
        {
            m_state.camera.MoveRight(moveDistance);
        }
        if (capabilities.window->IsKeyDown('Q'))
        {
            m_state.camera.MoveUp(-moveDistance);
        }
        if (capabilities.window->IsKeyDown('E'))
        {
            m_state.camera.MoveUp(moveDistance);
        }

        const Vector3 focusPoint = m_state.camera.target;
        if (capabilities.window->IsKeyDown(VK_LEFT))
        {
            m_state.camera.OrbitAround(focusPoint, -orbitStep, 0.0f);
        }
        if (capabilities.window->IsKeyDown(VK_RIGHT))
        {
            m_state.camera.OrbitAround(focusPoint, orbitStep, 0.0f);
        }
        if (capabilities.window->IsKeyDown(VK_UP))
        {
            m_state.camera.OrbitAround(focusPoint, 0.0f, orbitStep * 0.75f);
        }
        if (capabilities.window->IsKeyDown(VK_DOWN))
        {
            m_state.camera.OrbitAround(focusPoint, 0.0f, -orbitStep * 0.75f);
        }
        if (capabilities.window->IsKeyDown('Z'))
        {
            const Vector3 forward = m_state.camera.GetForward();
            const float currentDistance = Length(m_state.camera.target - m_state.camera.position);
            const float nextDistance = std::max(1.5f, currentDistance - (zoomSpeed * deltaTime));
            m_state.camera.position = m_state.camera.target - (forward * nextDistance);
        }
        if (capabilities.window->IsKeyDown('X'))
        {
            const Vector3 forward = m_state.camera.GetForward();
            const float currentDistance = Length(m_state.camera.target - m_state.camera.position);
            const float nextDistance = std::min(40.0f, currentDistance + (zoomSpeed * deltaTime));
            m_state.camera.position = m_state.camera.target - (forward * nextDistance);
        }
    }

    if (!m_state.rotationPaused)
    {
        m_state.liveSceneTime = capabilities.window->GetTotalTime() - m_state.pausedSceneTime;
    }
    else
    {
        m_state.pausedSceneTime = capabilities.window->GetTotalTime() - m_state.liveSceneTime;
    }
}

FramePrototype SandboxApp::BuildFrame() const
{
    SandboxScenePresetOptionsPrototype options;
    options.totalTime = m_state.liveSceneTime;
    options.rotationPaused = m_state.rotationPaused;
    options.cameraOrbitEnabled = m_state.orbitEnabled;
    options.cameraOverrideEnabled = !m_state.orbitEnabled;
    options.cameraControlCamera = m_state.camera;
    options.primarySceneLight = m_state.primarySceneLight;
    options.floorMaterial = &m_floorMaterial;
    options.roomMaterial = &m_roomMaterial;
    options.cubeMaterial = &m_cubeMaterial;
    options.emitterMaterial = &m_emitterMaterial;
    return SandboxScenePresetCompiler::BuildFrame(m_scene, options);
}

std::wstring SandboxApp::BuildOverlayText(const AppCapabilities& capabilities) const
{
    std::wostringstream overlay;
    overlay << L"DX LIGHTING LAB\n";
    overlay << L"backend: ";
    overlay << L"engine-runtime\n";
    overlay << L"scene: lighting lab\n";
    overlay << L"items: " << capabilities.rendering->GetSubmittedItemCount() << L"\n";
    overlay << L"frame: " << capabilities.rendering->GetFrameWidth() << L"x" << capabilities.rendering->GetFrameHeight() << L"\n";
    overlay << L"dt: " << capabilities.window->GetDeltaTime() << L" s\n";
    overlay << L"time: " << m_state.liveSceneTime << L" s\n";
    overlay << L"camera: " << (m_state.orbitEnabled ? L"orbit" : L"manual") << L"\n";
    overlay << L"exposure: " << m_state.camera.exposure << L"\n";
    overlay << L"light intensity: " << m_state.primarySceneLight.intensity << L"\n";
    overlay << L"light range: " << m_state.primarySceneLight.range << L"\n";
    overlay << L"animation: " << (m_state.rotationPaused ? L"paused" : L"running") << L"\n";
    overlay << L"materials: prototype-owned lighting lab set\n";
    overlay << L"[space] pause  [O] orbit/manual  [R] reset  [-/+] exposure\n";
    overlay << L"[J/K] light intensity  [N/M] light range\n";
    overlay << L"[WASD/QE] move  [arrows] orbit  [Z/X] zoom  [Shift] faster  [Esc] quit";
    return overlay.str();
}
