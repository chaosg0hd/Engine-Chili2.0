#include "moving_cube_sample_scene.hpp"

#include "../math/color_sample_math.hpp"
#include "../math/cube_sample_math.hpp"
#include "../math/math.hpp"

#include <array>
#include <cmath>

void MovingCubeSampleScenePrototype::SetTheoreticalWorkIterations(unsigned int iterations)
{
    m_theoreticalWorkIterations = iterations;
}

unsigned int MovingCubeSampleScenePrototype::GetTheoreticalWorkIterations() const
{
    return m_theoreticalWorkIterations;
}

std::uint32_t MovingCubeSampleScenePrototype::Sample(float screenX, float screenY, double time) const
{
    const float workSeed = screenX + (screenY * 1.37f) + static_cast<float>(time * 0.013);
    const float workNoise = ConsumeSampleWork(workSeed, m_theoreticalWorkIterations);

    static constexpr std::array<Vector3, 8> LocalVertices =
    {
        Vector3(-1.0f, -1.0f, -1.0f),
        Vector3( 1.0f, -1.0f, -1.0f),
        Vector3( 1.0f,  1.0f, -1.0f),
        Vector3(-1.0f,  1.0f, -1.0f),
        Vector3(-1.0f, -1.0f,  1.0f),
        Vector3( 1.0f, -1.0f,  1.0f),
        Vector3( 1.0f,  1.0f,  1.0f),
        Vector3(-1.0f,  1.0f,  1.0f)
    };
    static constexpr std::array<CubeFacePrototype, 6> Faces =
    {
        CubeFacePrototype{ { 0, 3, 2, 1 }, Vector3( 0.0f,  0.0f, -1.0f), 0xFF3967D7U },
        CubeFacePrototype{ { 4, 5, 6, 7 }, Vector3( 0.0f,  0.0f,  1.0f), 0xFFE15A48U },
        CubeFacePrototype{ { 0, 1, 5, 4 }, Vector3( 0.0f, -1.0f,  0.0f), 0xFF41A87BU },
        CubeFacePrototype{ { 3, 7, 6, 2 }, Vector3( 0.0f,  1.0f,  0.0f), 0xFFE0C24DU },
        CubeFacePrototype{ { 0, 4, 7, 3 }, Vector3(-1.0f,  0.0f,  0.0f), 0xFF8D5BD1U },
        CubeFacePrototype{ { 1, 2, 6, 5 }, Vector3( 1.0f,  0.0f,  0.0f), 0xFF52B9D5U }
    };

    std::array<CubeSampleVertexPrototype, 8> vertices;
    const Vector3 lightDirection = Normalize(Vector3(-0.45f, 0.68f, 0.58f));
    float bestDepth = -1000.0f;
    std::uint32_t bestColor = 0U;

    const auto sampleCube =
        [&](double cubeTime, float centerX, float centerY, float scale, float depthOffset, float colorScale)
        {
            for (std::size_t index = 0U; index < LocalVertices.size(); ++index)
            {
                vertices[index] = ProjectCubeSamplePoint(
                    RotateCubeSamplePoint(LocalVertices[index] * scale, cubeTime),
                    centerX,
                    centerY,
                    depthOffset);
            }

            for (const CubeFacePrototype& face : Faces)
            {
                if (!IsPointInCubeSampleFace(screenX, screenY, vertices, face))
                {
                    continue;
                }

                float faceDepth = 0.0f;
                for (int vertexIndex : face.vertices)
                {
                    faceDepth += vertices[static_cast<std::size_t>(vertexIndex)].z;
                }
                faceDepth *= 0.25f;
                if (faceDepth <= bestDepth)
                {
                    continue;
                }

                float closestEdge = 10.0f;
                for (std::size_t edge = 0U; edge < face.vertices.size(); ++edge)
                {
                    const CubeSampleVertexPrototype& a = vertices[static_cast<std::size_t>(face.vertices[edge])];
                    const CubeSampleVertexPrototype& b = vertices[static_cast<std::size_t>(face.vertices[(edge + 1U) % face.vertices.size()])];
                    closestEdge = std::min(closestEdge, ComputeDistanceToCubeSampleSegment(screenX, screenY, a, b));
                }

                const Vector3 rotatedNormal = Normalize(RotateCubeSamplePoint(face.normal, cubeTime));
                const float diffuse = std::max(0.0f, Dot(rotatedNormal, lightDirection));
                const float facing = std::max(0.0f, rotatedNormal.z);
                float shade = 0.24f + (diffuse * 0.62f) + (facing * 0.14f);
                shade *= colorScale * (0.985f + (std::fmod(workNoise, 1.0f) * 0.030f));
                if (closestEdge < 0.014f)
                {
                    shade *= 0.36f;
                }

                bestDepth = faceDepth;
                bestColor = ScaleRgbPackedColor(face.color, shade);
            }
        };

    const float timeValue = static_cast<float>(time);
    sampleCube(
        time * 1.04,
        std::sin(timeValue * 0.42f) * 1.12f,
        std::cos(timeValue * 0.31f) * 0.62f,
        1.12f,
        0.58f,
        1.00f);
    sampleCube(
        (time * 1.33) + 1.70,
        -0.35f + (std::sin(timeValue * 0.58f + 1.2f) * 1.28f),
        0.12f + (std::cos(timeValue * 0.44f + 0.7f) * 0.76f),
        0.78f,
        0.16f,
        0.88f);
    sampleCube(
        (time * 0.82) + 3.20,
        0.55f + (std::cos(timeValue * 0.37f + 2.1f) * 1.38f),
        -0.18f + (std::sin(timeValue * 0.66f + 1.4f) * 0.88f),
        0.92f,
        0.34f,
        0.94f);
    sampleCube(
        (time * 1.58) + 4.60,
        std::sin(timeValue * 0.74f + 2.6f) * 1.52f,
        -0.08f + (std::cos(timeValue * 0.53f + 3.5f) * 0.95f),
        0.62f,
        -0.06f,
        0.80f);
    sampleCube(
        (time * 0.69) + 6.30,
        -0.15f + (std::cos(timeValue * 0.29f + 4.1f) * 1.68f),
        std::sin(timeValue * 0.49f + 2.9f) * 0.72f,
        1.24f,
        0.70f,
        1.06f);
    sampleCube(
        (time * 1.12) + 8.10,
        0.20f + (std::sin(timeValue * 0.91f + 5.2f) * 1.42f),
        0.08f + (std::cos(timeValue * 0.82f + 4.6f) * 1.08f),
        0.54f,
        0.04f,
        0.76f);

    if (bestColor != 0U)
    {
        return bestColor;
    }

    const float vignette = ClampUnitFloat(1.0f - (std::sqrt((screenX * screenX) + (screenY * screenY)) * 0.36f));
    const float pulse = 0.5f + (std::sin(static_cast<float>(time * 1.7) + (screenX * 5.0f)) * 0.5f);
    return PackRgbColor(
        (0.025f + (pulse * 0.020f)) * vignette,
        (0.035f + (screenY * 0.010f)) * vignette,
        (0.045f + ((1.0f - pulse) * 0.018f)) * vignette);
}
