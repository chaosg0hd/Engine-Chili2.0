#pragma once

#include <array>
#include <cstdint>

namespace render_builtin_meshes
{
    struct Vertex
    {
        float x;
        float y;
        float z;
        float nx;
        float ny;
        float nz;
        float u;
        float v;
    };

    constexpr std::array<Vertex, 3> kTriangleVertices =
    {{
        { 0.0f, 0.45f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f },
        { 0.45f, -0.35f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
        { -0.45f, -0.35f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f }
    }};

    constexpr std::array<std::uint16_t, 3> kTriangleIndices = {{ 0, 1, 2 }};

    constexpr std::array<Vertex, 4> kDiamondVertices =
    {{
        { 0.0f, 0.50f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f },
        { 0.40f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f },
        { 0.0f, -0.50f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f },
        { -0.40f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f }
    }};

    constexpr std::array<std::uint16_t, 6> kDiamondIndices = {{ 0, 1, 2, 0, 2, 3 }};

    constexpr std::array<Vertex, 4> kQuadVertices =
    {{
        { -0.35f, 0.35f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f },
        { 0.35f, 0.35f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f },
        { 0.35f, -0.35f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f },
        { -0.35f, -0.35f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f }
    }};

    constexpr std::array<std::uint16_t, 6> kQuadIndices = {{ 0, 1, 2, 0, 2, 3 }};

    constexpr std::array<Vertex, 7> kHexVertices =
    {{
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f },
        { 0.35f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f },
        { 0.175f, 0.3031089f, 0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 0.0f },
        { -0.175f, 0.3031089f, 0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 0.0f },
        { -0.35f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f },
        { -0.175f, -0.3031089f, 0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f },
        { 0.175f, -0.3031089f, 0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f }
    }};

    constexpr std::array<std::uint16_t, 18> kHexIndices =
    {{
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
        0, 4, 5,
        0, 5, 6,
        0, 6, 1
    }};

    constexpr std::array<Vertex, 24> kCubeVertices =
    {{
        { -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
        {  0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f },
        {  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
        { -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f },

        {  0.5f,  0.5f, -0.5f, 0.0f, 0.0f,-1.0f, 0.0f, 0.0f },
        { -0.5f,  0.5f, -0.5f, 0.0f, 0.0f,-1.0f, 1.0f, 0.0f },
        { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,-1.0f, 1.0f, 1.0f },
        {  0.5f, -0.5f, -0.5f, 0.0f, 0.0f,-1.0f, 0.0f, 1.0f },

        { -0.5f,  0.5f, -0.5f,-1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        { -0.5f,  0.5f,  0.5f,-1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
        { -0.5f, -0.5f,  0.5f,-1.0f, 0.0f, 0.0f, 1.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f,-1.0f, 0.0f, 0.0f, 0.0f, 1.0f },

        {  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        {  0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
        {  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f },
        {  0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f },

        { -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f },
        {  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f },
        {  0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f },
        { -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },

        { -0.5f, -0.5f,  0.5f, 0.0f,-1.0f, 0.0f, 0.0f, 0.0f },
        {  0.5f, -0.5f,  0.5f, 0.0f,-1.0f, 0.0f, 1.0f, 0.0f },
        {  0.5f, -0.5f, -0.5f, 0.0f,-1.0f, 0.0f, 1.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f, 0.0f,-1.0f, 0.0f, 0.0f, 1.0f }
    }};

    constexpr std::array<std::uint16_t, 36> kCubeIndices =
    {{
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    }};

    constexpr std::array<Vertex, 6> kOctahedronVertices =
    {{
        { 0.0f, 0.65f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f },
        { 0.65f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f },
        { 0.0f, 0.0f, 0.65f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f },
        { -0.65f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.5f },
        { 0.0f, 0.0f, -0.65f, 0.0f, 0.0f, -1.0f, 0.5f, 1.0f },
        { 0.0f, -0.65f, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f }
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
