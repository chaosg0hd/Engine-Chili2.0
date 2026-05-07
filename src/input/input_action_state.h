#pragma once

struct InputActionState
{
    bool pressed = false;
    bool down = false;
    bool released = false;
    float value = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
};
