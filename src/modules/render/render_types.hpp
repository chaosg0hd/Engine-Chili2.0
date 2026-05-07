#pragma once

#include <algorithm>
#include <cstdint>

enum class RenderBackendType
{
    Null = 0,
    DirectX11,
    DirectX12,
    Vulkan,
    OpenGL
};

enum class RenderResult
{
    Success = 0,
    Failure
};

struct ViewportRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    float Aspect() const
    {
        return (height > 0) ? (static_cast<float>(width) / static_cast<float>(height)) : 1.0f;
    }

    bool Contains(int px, int py) const
    {
        return px >= x &&
            py >= y &&
            px < (x + std::max(0, width)) &&
            py < (y + std::max(0, height));
    }
};

using RenderViewport = ViewportRect;

struct RenderClearColor
{
    float r = 0.08f;
    float g = 0.08f;
    float b = 0.10f;
    float a = 1.0f;
};

struct DerivedBounceFillSettings
{
    bool enabled = true;
    float bounceStrength = 0.14f;
    float environmentTint[3] = { 1.0f, 1.0f, 1.0f };
    bool shadowAwareBounce = true;
};

struct TracedIndirectSettings
{
    bool enabled = true;
    float bounceStrength = 0.30f;
    float maxTraceDistance = 10.0f;
};
