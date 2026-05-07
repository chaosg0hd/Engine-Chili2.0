#include "template_app.hpp"

#include "core/engine_core.hpp"
#include "native_ui/native_ui_builder.hpp"
#include "template_frame_builder.hpp"

#include <algorithm>

namespace
{
    constexpr std::uint32_t kClearColor = 0xFF081018u;

    float ReadAxis(const IAppInput& input, AppKey positive, AppKey negative)
    {
        float axis = 0.0f;
        if (input.IsKeyDown(positive))
        {
            axis += 1.0f;
        }
        if (input.IsKeyDown(negative))
        {
            axis -= 1.0f;
        }

        return std::clamp(axis, -1.0f, 1.0f);
    }
}

bool TemplateApp::Run()
{
    EngineCore core;
    core.SetStartupWindowTitle(L"Engine App Template");
    core.SetStartupWindowSize(1280, 720);
    if (!core.Initialize())
    {
        return false;
    }

    AppCapabilities& capabilities = core.GetAppCapabilities();
    Initialize(capabilities);

    // EngineCore owns lifecycle and frame orchestration. The app only supplies
    // per-frame intent through this callback.
    core.SetFrameCallback(
        [this](AppCapabilities& callbackCapabilities)
        {
            UpdateFrame(callbackCapabilities);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void TemplateApp::Initialize(AppCapabilities& capabilities)
{
    TemplateGameSystem::Reset(m_game);
    if (capabilities.logging)
    {
        capabilities.logging->Info("TemplateApp ready.");
    }
}

void TemplateApp::UpdateFrame(AppCapabilities& capabilities)
{
    const TemplateInput input = ReadGameplayInput(*capabilities.input);
    SimulateGame(input, *capabilities.time);
    PresentGame(capabilities);
}

TemplateInput TemplateApp::ReadGameplayInput(const IAppInput& input) const
{
    TemplateInput gameplayInput;
    gameplayInput.moveAxis = ReadAxis(input, AppKey::Right, AppKey::Left);
    gameplayInput.actionPressed = input.WasKeyPressed(AppKey::Space);
    gameplayInput.resetPressed = input.WasKeyPressed(AppKey::R);
    return gameplayInput;
}

void TemplateApp::SimulateGame(const TemplateInput& input, const IAppTime& time)
{
    TemplateGameSystem::Step(
        m_game,
        input,
        static_cast<float>(std::max(0.0, time.GetDeltaSeconds())));
}

void TemplateApp::PresentGame(AppCapabilities& capabilities) const
{
    NativeUiBuilder ui;
    ui.WindowTitle(L"Engine App Template")
        .OverlayEnabled(false)
        .ClearColor(kClearColor)
        .ContentFrame(TemplateFrameBuilder::Build(m_game));

    // Native UI is for HUD/status/help panels. It is pixel/anchor based and
    // should not be mixed into render-frame game geometry.
    ui.Panel("Template")
        .Anchor(NativeUiAnchor::TopLeft, 16, 16, 300, 110)
        .Colors(0xFFE8F3F1u, 0xDD081018u)
        .Row("Score", m_game.score)
        .Row("Player X", m_game.playerX)
        .Row("Controls", "Left/Right, Space, R, Esc");

    capabilities.nativeUi->Submit(ui.Build());
}
