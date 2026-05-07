#pragma once

#include "input_key.h"
#include "input_mouse.h"

#include <array>

struct RawButtonState
{
    bool pressed = false;
    bool down = false;
    bool released = false;
};

struct RawInputState
{
    int mouseX = 0;
    int mouseY = 0;
    int mouseDeltaX = 0;
    int mouseDeltaY = 0;
    int mouseWheelDelta = 0;
    std::array<RawButtonState, static_cast<std::size_t>(InputMouseButton::Count)> mouseButtons{};
    std::array<RawButtonState, static_cast<std::size_t>(InputKey::Count)> keys{};

    const RawButtonState& Key(InputKey key) const
    {
        return keys[static_cast<std::size_t>(key)];
    }

    const RawButtonState& Mouse(InputMouseButton button) const
    {
        return mouseButtons[static_cast<std::size_t>(button)];
    }
};
