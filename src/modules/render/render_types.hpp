#pragma once

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

struct RenderViewport
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

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
