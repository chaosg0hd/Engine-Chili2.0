#pragma once

#include "input_key.h"
#include "input_mouse.h"

enum class InputBindingKind
{
    Key,
    MouseButton,
    MouseWheel
};

struct InputBinding
{
    InputBindingKind kind = InputBindingKind::Key;
    InputKey key = InputKey::Unknown;
    InputMouseButton mouseButton = InputMouseButton::Left;
    bool requireCtrl = false;
    bool requireShift = false;
    bool requireAlt = false;
    bool consume = true;
};

inline InputBinding BindKey(InputKey key, bool ctrl = false, bool shift = false, bool alt = false, bool consume = true)
{
    InputBinding binding;
    binding.kind = InputBindingKind::Key;
    binding.key = key;
    binding.requireCtrl = ctrl;
    binding.requireShift = shift;
    binding.requireAlt = alt;
    binding.consume = consume;
    return binding;
}

inline InputBinding BindMouseButton(
    InputMouseButton button,
    bool ctrl = false,
    bool shift = false,
    bool alt = false,
    bool consume = true)
{
    InputBinding binding;
    binding.kind = InputBindingKind::MouseButton;
    binding.mouseButton = button;
    binding.requireCtrl = ctrl;
    binding.requireShift = shift;
    binding.requireAlt = alt;
    binding.consume = consume;
    return binding;
}

inline InputBinding BindMouseWheel(bool ctrl = false, bool shift = false, bool alt = false, bool consume = true)
{
    InputBinding binding;
    binding.kind = InputBindingKind::MouseWheel;
    binding.requireCtrl = ctrl;
    binding.requireShift = shift;
    binding.requireAlt = alt;
    binding.consume = consume;
    return binding;
}
