#include "game_host.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "native_ui/native_ui_builder.hpp"
#include "input/input_key.h"
#include "input/input_mouse.h"

#include <algorithm>
#include <filesystem>
#include <string>

bool GameHost::Initialize()
{
    if (!m_bridge.Initialize())
        return false;

    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    const std::string exeDir =
        std::filesystem::path(exePath).parent_path().string();

    m_sceneManager.AddScene(0, exeDir + "/scene0.scene.json");
    m_sceneManager.AddScene(1, exeDir + "/scene1.scene.json");

    AppCapabilities& capabilities = m_bridge.GetCapabilities();
    m_runtimeHost.SetInputInterface(capabilities.input);

    if (!LoadScene(0))
    {
        m_bridge.Shutdown();
        return false;
    }

    m_bridge.SetEscapeShutdownEnabled(false);
    m_initialized = true;
    return true;
}

bool GameHost::LoadScene(int index)
{
    const std::string& path = m_sceneManager.GetScenePath(index);
    if (path.empty())
    {
        m_bridge.LogError("GameHost: no scene at index " + std::to_string(index));
        return false;
    }

    m_runtimeHost.Stop();

    std::string sceneError;
    m_runtimeHost.Initialize(path, sceneError);
    if (!sceneError.empty())
        m_bridge.LogWarn("GameHost: scene " + std::to_string(index) + " warning: " + sceneError);

    studio_runtime::ProjectRuntimeDesc project;
    project.projectId          = "pong";
    project.projectName        = "Pong";
    project.previewRuntimeName = "StudioPreviewRuntime";

    std::string playError;
    if (!m_runtimeHost.Play(project, playError))
    {
        m_bridge.LogError("GameHost: play failed on scene " + std::to_string(index) + ": " + playError);
        return false;
    }

    m_sceneManager.SetCurrentScene(index);
    return true;
}

void GameHost::Run()
{
    if (!m_initialized)
        return;

    while (!m_bridge.ShouldExit())
    {
        if (!m_bridge.Tick())
            break;

        TickGame();
    }
}

void GameHost::TickGame()
{
    AppCapabilities& capabilities = m_bridge.GetCapabilities();

    // Build input snapshot for the runtime (pong game controls on scene 1).
    studio_runtime::RuntimeInput input;
    if (capabilities.input)
    {
        auto setKey = [&](InputKey key, AppKey appKey)
        {
            RawButtonState& s = input.rawInput.keys[static_cast<std::size_t>(key)];
            s.pressed  = capabilities.input->WasKeyPressed(appKey);
            s.down     = capabilities.input->IsKeyDown(appKey);
            s.released = capabilities.input->WasKeyReleased(appKey);
        };
        setKey(InputKey::W,    AppKey::W);
        setKey(InputKey::S,    AppKey::S);
        setKey(InputKey::Up,   AppKey::Up);
        setKey(InputKey::Down, AppKey::Down);
        // Escape from pong (scene 1) goes back to menu; SceneManagerScript handles menu escape.
        if (m_sceneManager.GetCurrentScene() == 1 &&
            capabilities.input->WasKeyPressed(static_cast<AppKey>(0x1B)))
        {
            m_sceneManager.RequestTransition(0);
        }
    }

    const float dt = capabilities.time
        ? static_cast<float>(capabilities.time->GetDeltaSeconds())
        : (1.0f / 60.0f);

    int w = 1280, h = 720;
    if (capabilities.window)
    {
        w = std::max(1, capabilities.window->GetWindowWidth());
        h = std::max(1, capabilities.window->GetWindowHeight());
    }

    ViewportRect viewport{0, 0, w, h};
    m_runtimeHost.Tick(dt, input, viewport);

    // Consume scene signals emitted by scripts this frame.
    const int transition = m_runtimeHost.ConsumeSceneTransition();
    if (transition >= 0)
    {
        LoadScene(transition);
        return;
    }
    if (m_runtimeHost.ConsumeExitRequest())
    {
        m_bridge.RequestExit();
        return;
    }

    if (!capabilities.nativeUi)
        return;

    NativeUiBuilder ui;
    ui.OverlayEnabled(false).ClearColor(0xFF101820u);

    if (m_sceneManager.GetCurrentScene() == 0)
    {
        ui.WindowTitle(L"Pong — Menu");
        if (m_runtimeHost.HasRenderFrame())
            ui.ContentFrame(m_runtimeHost.GetRenderFrame());

        ui.Panel("Menu")
            .Anchor(NativeUiAnchor::Center, 0, 0, 300, 108)
            .Colors(0xFFFFF2E4u, 0xDD241A18u)
            .Row("PONG", "")
            .Row("Play", "SPACE")
            .Row("Exit", "ESC");
    }
    else
    {
        ui.WindowTitle(L"Pong");
        if (m_runtimeHost.HasRenderFrame())
            ui.ContentFrame(m_runtimeHost.GetRenderFrame());

        ui.Panel("Hint")
            .Anchor(NativeUiAnchor::TopLeft, 8, 8, 160, 40)
            .Colors(0xFFFFF2E4u, 0x99241A18u)
            .Row("ESC", "Menu");
    }

    capabilities.nativeUi->Submit(ui.Build());
}

void GameHost::Shutdown()
{
    if (!m_initialized)
        return;

    m_bridge.Shutdown();
    m_initialized = false;
}
