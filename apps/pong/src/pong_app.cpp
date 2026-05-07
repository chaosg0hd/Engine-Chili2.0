#include "pong_app.hpp"

#include "core/engine_core.hpp"
#include "native_ui/native_ui_builder.hpp"
#include "pong_frame_builder.hpp"

#include <algorithm>
#include <string>

namespace
{
    const ColorPrototype kPongClearColor = ColorPrototype::FromBytes(7, 16, 15);
    const ColorPrototype kPanelTextColor = ColorPrototype::FromBytes(232, 243, 241);
    const ColorPrototype kPanelBackgroundColor = ColorPrototype::FromBytes(7, 16, 15, 221);

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

    bool WasPressed(const IAppInput& input, AppKey key)
    {
        return input.WasKeyPressed(key);
    }
}

bool PongApp::Run()
{
    EngineCore core;
    core.SetStartupWindowTitle(L"Engine Pong");
    core.SetStartupWindowSize(1280, 720);
    if (!core.Initialize())
    {
        return false;
    }

    AppCapabilities& capabilities = core.GetAppCapabilities();
    Initialize(capabilities);

    core.SetFrameCallback(
        [this](AppCapabilities& callbackCapabilities)
        {
            UpdateFrame(callbackCapabilities);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void PongApp::Initialize(AppCapabilities& capabilities)
{
    PongGameSystem::Reset(m_game);
    if (capabilities.logging)
    {
        capabilities.logging->Info("PongApp: prototype-driven Pong game ready.");
    }
}

void PongApp::UpdateFrame(AppCapabilities& capabilities)
{
    ApplyAppActions(*capabilities.input);

    const PongInput input = ReadGameplayInput(*capabilities.input);
    SimulateGame(input, *capabilities.time);
    PresentGame(capabilities);
}

void PongApp::ApplyAppActions(const IAppInput& input)
{
    if (WasPressed(input, AppKey::A))
    {
        m_rightPaddleAiEnabled = !m_rightPaddleAiEnabled;
    }

    if (WasPressed(input, AppKey::L))
    {
        m_leftPaddleAiEnabled = !m_leftPaddleAiEnabled;
    }
}

PongInput PongApp::ReadGameplayInput(const IAppInput& input) const
{
    PongInput gameplayInput;
    gameplayInput.leftPaddleAxis = ReadAxis(input, AppKey::W, AppKey::S);
    gameplayInput.rightPaddleAxis = ReadAxis(input, AppKey::Up, AppKey::Down);
    gameplayInput.leftPaddleAiEnabled = m_leftPaddleAiEnabled;
    gameplayInput.rightPaddleAiEnabled = m_rightPaddleAiEnabled;
    gameplayInput.resetRequested = WasPressed(input, AppKey::R);
    gameplayInput.serveRequested = WasPressed(input, AppKey::Space);
    return gameplayInput;
}

void PongApp::SimulateGame(const PongInput& input, const IAppTime& time)
{
    PongGameSystem::Step(
        m_game,
        input,
        static_cast<float>(std::max(0.0, time.GetDeltaSeconds())));
}

void PongApp::PresentGame(AppCapabilities& capabilities) const
{
    NativeUiBuilder ui;
    ui.WindowTitle(L"Engine Pong")
        .OverlayEnabled(false)
        .ClearColor(kPongClearColor)
        .ContentFrame(PongFrameBuilder::Build(m_game));

    ui.Panel("Pong State")
        .Anchor(NativeUiAnchor::TopLeft, 16, 16, 330, 142)
        .Colors(kPanelTextColor, kPanelBackgroundColor)
        .Row("Left Score", m_game.score.left)
        .Row("Right Score", m_game.score.right)
        .Row("Rally", m_game.rallyCount)
        .Row("Target", m_game.rules.winningScore)
        .Row("Left AI", m_leftPaddleAiEnabled ? "on" : "off")
        .Row("Right AI", m_rightPaddleAiEnabled ? "on" : "off")
        .Row("Ball", m_game.ball.waitingForServe ? "ready" : "live")
        .Row("Controls", "W/S, Up/Down, L, A, Space, R, Esc");

    capabilities.nativeUi->Submit(ui.Build());
}
