#pragma once

#include <algorithm>
#include <cstdint>

inline float ClampUnitFloat(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

inline std::uint32_t PackRgbColor(float red, float green, float blue)
{
    const auto toByte =
        [](float value) -> std::uint32_t
        {
            return static_cast<std::uint32_t>((ClampUnitFloat(value) * 255.0f) + 0.5f);
        };

    return 0xFF000000U | (toByte(red) << 16U) | (toByte(green) << 8U) | toByte(blue);
}

inline void UnpackRgbColor(std::uint32_t color, float& red, float& green, float& blue)
{
    red = static_cast<float>((color >> 16U) & 0xFFU) / 255.0f;
    green = static_cast<float>((color >> 8U) & 0xFFU) / 255.0f;
    blue = static_cast<float>(color & 0xFFU) / 255.0f;
}

inline std::uint32_t ScaleRgbPackedColor(std::uint32_t color, float scale)
{
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    UnpackRgbColor(color, red, green, blue);
    return PackRgbColor(red * scale, green * scale, blue * scale);
}
