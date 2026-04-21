#pragma once

#include <cmath>
#include <cstdint>

struct HexGridCoordPrototype
{
    int q = 0;
    int r = 0;

    constexpr HexGridCoordPrototype() = default;
    constexpr HexGridCoordPrototype(int inQ, int inR)
        : q(inQ), r(inR)
    {
    }
};

inline std::int64_t PackHexGridKey(const HexGridCoordPrototype& hex)
{
    const std::uint64_t q = static_cast<std::uint32_t>(hex.q);
    const std::uint64_t r = static_cast<std::uint32_t>(hex.r);
    return static_cast<std::int64_t>((q << 32U) | r);
}

inline int ComputeHexGridDistance(
    const HexGridCoordPrototype& first,
    const HexGridCoordPrototype& second)
{
    const int dq = first.q - second.q;
    const int dr = first.r - second.r;
    const int ds = (-first.q - first.r) - (-second.q - second.r);
    return (std::abs(dq) + std::abs(dr) + std::abs(ds)) / 2;
}

inline HexGridCoordPrototype RoundHexGridAxial(float q, float r)
{
    float cubeX = q;
    float cubeZ = r;
    float cubeY = -cubeX - cubeZ;

    int roundedX = static_cast<int>(std::round(cubeX));
    int roundedY = static_cast<int>(std::round(cubeY));
    int roundedZ = static_cast<int>(std::round(cubeZ));

    const float xDiff = std::fabs(static_cast<float>(roundedX) - cubeX);
    const float yDiff = std::fabs(static_cast<float>(roundedY) - cubeY);
    const float zDiff = std::fabs(static_cast<float>(roundedZ) - cubeZ);

    if (xDiff > yDiff && xDiff > zDiff)
    {
        roundedX = -roundedY - roundedZ;
    }
    else if (yDiff > zDiff)
    {
        roundedY = -roundedX - roundedZ;
    }
    else
    {
        roundedZ = -roundedX - roundedY;
    }

    return HexGridCoordPrototype{ roundedX, roundedZ };
}

inline HexGridCoordPrototype RotateHexGridOffset30Degrees(const HexGridCoordPrototype& offset)
{
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float Sqrt3 = 1.73205080756887729353f;
    constexpr float ThirtyDegrees = Pi / 6.0f;

    const float x = 1.5f * static_cast<float>(offset.q);
    const float y = Sqrt3 * (static_cast<float>(offset.r) + (static_cast<float>(offset.q) * 0.5f));
    const float rotatedX = (x * std::cos(ThirtyDegrees)) - (y * std::sin(ThirtyDegrees));
    const float rotatedY = (x * std::sin(ThirtyDegrees)) + (y * std::cos(ThirtyDegrees));
    const float q = (2.0f / 3.0f) * rotatedX;
    const float r = (rotatedY / Sqrt3) - (q * 0.5f);
    return RoundHexGridAxial(q, r);
}
