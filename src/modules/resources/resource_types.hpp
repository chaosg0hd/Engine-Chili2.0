#pragma once

#include <cstdint>

enum class ResourceKind : std::uint8_t
{
    Unknown = 0,
    Texture,
    Mesh,
    Material,
    Shader,
    PrototypeJson
};

enum class ResourceState : std::uint8_t
{
    Unloaded = 0,
    Queued,
    Loading,
    Processing,
    Uploading,
    Ready,
    Failed
};

using ResourceHandle = std::uint32_t;
