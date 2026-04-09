#pragma once

#include <array>
#include <cstdint>

namespace sandbox_builtin_meshes
{
    struct Vertex
    {
        float x;
        float y;
        float z;
    };

    constexpr std::array<Vertex, 3> kTriangleVertices =
    {{
        { 0.0f, 0.45f, 0.0f },
        { 0.45f, -0.35f, 0.0f },
        { -0.45f, -0.35f, 0.0f }
    }};

    constexpr std::array<std::uint16_t, 3> kTriangleIndices = {{ 0, 1, 2 }};

    constexpr std::array<Vertex, 4> kDiamondVertices =
    {{
        { 0.0f, 0.50f, 0.0f },
        { 0.40f, 0.0f, 0.0f },
        { 0.0f, -0.50f, 0.0f },
        { -0.40f, 0.0f, 0.0f }
    }};

    constexpr std::array<std::uint16_t, 6> kDiamondIndices = {{ 0, 1, 2, 0, 2, 3 }};

    constexpr std::array<Vertex, 4> kQuadVertices =
    {{
        { -0.35f, 0.35f, 0.0f },
        { 0.35f, 0.35f, 0.0f },
        { 0.35f, -0.35f, 0.0f },
        { -0.35f, -0.35f, 0.0f }
    }};

    constexpr std::array<std::uint16_t, 6> kQuadIndices = {{ 0, 1, 2, 0, 2, 3 }};

    constexpr std::array<Vertex, 8> kCubeVertices =
    {{
        { -0.5f, -0.5f, -0.5f },
        { 0.5f, -0.5f, -0.5f },
        { 0.5f, 0.5f, -0.5f },
        { -0.5f, 0.5f, -0.5f },
        { -0.5f, -0.5f, 0.5f },
        { 0.5f, -0.5f, 0.5f },
        { 0.5f, 0.5f, 0.5f },
        { -0.5f, 0.5f, 0.5f }
    }};

    constexpr std::array<std::uint16_t, 36> kCubeIndices =
    {{
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    }};

    constexpr std::array<Vertex, 6> kOctahedronVertices =
    {{
        { 0.0f, 0.65f, 0.0f },
        { 0.65f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.65f },
        { -0.65f, 0.0f, 0.0f },
        { 0.0f, 0.0f, -0.65f },
        { 0.0f, -0.65f, 0.0f }
    }};

    constexpr std::array<std::uint16_t, 24> kOctahedronIndices =
    {{
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
        0, 4, 1,
        5, 2, 1,
        5, 3, 2,
        5, 4, 3,
        5, 1, 4
    }};
}
