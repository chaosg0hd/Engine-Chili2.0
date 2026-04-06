#pragma once

#include "../../core/module.hpp"

#include <array>
#include <cstdint>

class EngineContext;
class PlatformModule;

class InputModule : public IModule
{
public:
    static constexpr std::size_t KeyCount = 256;

    enum class MouseButton : std::size_t
    {
        Left = 0,
        Right,
        Middle,
        Count
    };

public:
    InputModule();

    const char* GetName() const override;
    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    void SetPlatformModule(PlatformModule* platform);

    bool IsKeyDown(std::uint8_t key) const;
    bool WasKeyPressed(std::uint8_t key) const;
    bool WasKeyReleased(std::uint8_t key) const;
    bool IsMouseButtonDown(MouseButton button) const;
    bool WasMouseButtonPressed(MouseButton button) const;
    bool WasMouseButtonReleased(MouseButton button) const;
    int GetMouseX() const;
    int GetMouseY() const;
    int GetMouseDeltaX() const;
    int GetMouseDeltaY() const;
    int GetMouseWheelDelta() const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    void BeginFrameState();
    void ProcessPlatformEvents();

private:
    bool m_initialized = false;
    bool m_started = false;

    PlatformModule* m_platform = nullptr;

    std::array<bool, KeyCount> m_currentKeys{};
    std::array<bool, KeyCount> m_previousKeys{};
    std::array<bool, KeyCount> m_pressedKeys{};
    std::array<bool, KeyCount> m_releasedKeys{};
    std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_currentMouseButtons{};
    std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_previousMouseButtons{};
    std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_pressedMouseButtons{};
    std::array<bool, static_cast<std::size_t>(MouseButton::Count)> m_releasedMouseButtons{};
    int m_mouseX = 0;
    int m_mouseY = 0;
    int m_previousMouseX = 0;
    int m_previousMouseY = 0;
    int m_mouseDeltaX = 0;
    int m_mouseDeltaY = 0;
    int m_mouseWheelDelta = 0;
};
