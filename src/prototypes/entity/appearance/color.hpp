#pragma once

#include <algorithm>
#include <cstdint>

struct ColorPrototype
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    constexpr ColorPrototype() = default;

    constexpr ColorPrototype(float inR, float inG, float inB, float inA = 1.0f)
        : r(inR),
          g(inG),
          b(inB),
          a(inA)
    {
    }

    static ColorPrototype FromBytes(
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue,
        std::uint8_t alpha = 255)
    {
        constexpr float byteToUnit = 1.0f / 255.0f;
        return ColorPrototype(
            static_cast<float>(red) * byteToUnit,
            static_cast<float>(green) * byteToUnit,
            static_cast<float>(blue) * byteToUnit,
            static_cast<float>(alpha) * byteToUnit);
    }

    static ColorPrototype FromArgb(std::uint32_t argb)
    {
        return FromBytes(
            static_cast<std::uint8_t>((argb >> 16) & 0xFFu),
            static_cast<std::uint8_t>((argb >> 8) & 0xFFu),
            static_cast<std::uint8_t>(argb & 0xFFu),
            static_cast<std::uint8_t>((argb >> 24) & 0xFFu));
    }

    std::uint32_t ToArgb() const
    {
        const auto toByte =
            [](float value)
            {
                const float clamped = std::clamp(value, 0.0f, 1.0f);
                return static_cast<std::uint32_t>((clamped * 255.0f) + 0.5f);
            };

        return (toByte(a) << 24) |
               (toByte(r) << 16) |
               (toByte(g) << 8) |
               toByte(b);
    }

    ColorPrototype WithAlpha(float alpha) const
    {
        return ColorPrototype(r, g, b, alpha);
    }
};
