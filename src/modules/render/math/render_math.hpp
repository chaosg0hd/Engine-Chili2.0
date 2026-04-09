#pragma once

#include <cmath>

struct RenderVector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr RenderVector3() = default;
    constexpr RenderVector3(float inX, float inY, float inZ)
        : x(inX), y(inY), z(inZ)
    {
    }

    constexpr RenderVector3 operator+(const RenderVector3& other) const
    {
        return RenderVector3(x + other.x, y + other.y, z + other.z);
    }

    constexpr RenderVector3 operator-(const RenderVector3& other) const
    {
        return RenderVector3(x - other.x, y - other.y, z - other.z);
    }

    constexpr RenderVector3 operator*(float scalar) const
    {
        return RenderVector3(x * scalar, y * scalar, z * scalar);
    }
};

inline float RenderDot(const RenderVector3& a, const RenderVector3& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

inline RenderVector3 RenderCross(const RenderVector3& a, const RenderVector3& b)
{
    return RenderVector3(
        (a.y * b.z) - (a.z * b.y),
        (a.z * b.x) - (a.x * b.z),
        (a.x * b.y) - (a.y * b.x));
}

inline float RenderLength(const RenderVector3& value)
{
    return std::sqrt(RenderDot(value, value));
}

inline RenderVector3 RenderNormalize(const RenderVector3& value)
{
    const float length = RenderLength(value);
    if (length <= 0.000001f)
    {
        return RenderVector3{};
    }

    return value * (1.0f / length);
}

struct RenderMatrix4
{
    float m[4][4] =
    {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };

    static RenderMatrix4 Identity()
    {
        return RenderMatrix4{};
    }
};

inline RenderMatrix4 RenderMultiply(const RenderMatrix4& a, const RenderMatrix4& b)
{
    RenderMatrix4 result = {};

    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            result.m[row][column] = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                result.m[row][column] += a.m[row][k] * b.m[k][column];
            }
        }
    }

    return result;
}

inline RenderMatrix4 RenderCreateTranslationMatrix(const RenderVector3& translation)
{
    RenderMatrix4 result = RenderMatrix4::Identity();
    result.m[3][0] = translation.x;
    result.m[3][1] = translation.y;
    result.m[3][2] = translation.z;
    return result;
}

inline RenderMatrix4 RenderCreateScaleMatrix(const RenderVector3& scale)
{
    RenderMatrix4 result = RenderMatrix4::Identity();
    result.m[0][0] = scale.x;
    result.m[1][1] = scale.y;
    result.m[2][2] = scale.z;
    return result;
}

inline RenderMatrix4 RenderCreateRotationXMatrix(float radians)
{
    RenderMatrix4 result = RenderMatrix4::Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    result.m[1][1] = c;
    result.m[1][2] = s;
    result.m[2][1] = -s;
    result.m[2][2] = c;
    return result;
}

inline RenderMatrix4 RenderCreateRotationYMatrix(float radians)
{
    RenderMatrix4 result = RenderMatrix4::Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    result.m[0][0] = c;
    result.m[0][2] = -s;
    result.m[2][0] = s;
    result.m[2][2] = c;
    return result;
}

inline RenderMatrix4 RenderCreateRotationZMatrix(float radians)
{
    RenderMatrix4 result = RenderMatrix4::Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    result.m[0][0] = c;
    result.m[0][1] = s;
    result.m[1][0] = -s;
    result.m[1][1] = c;
    return result;
}

inline RenderMatrix4 RenderCreateLookAtMatrix(
    const RenderVector3& eye,
    const RenderVector3& target,
    const RenderVector3& up)
{
    const RenderVector3 forward = RenderNormalize(target - eye);
    const RenderVector3 right = RenderNormalize(RenderCross(up, forward));
    const RenderVector3 cameraUp = RenderCross(forward, right);

    RenderMatrix4 result = {};
    result.m[0][0] = right.x;
    result.m[1][0] = right.y;
    result.m[2][0] = right.z;
    result.m[3][0] = -RenderDot(right, eye);

    result.m[0][1] = cameraUp.x;
    result.m[1][1] = cameraUp.y;
    result.m[2][1] = cameraUp.z;
    result.m[3][1] = -RenderDot(cameraUp, eye);

    result.m[0][2] = forward.x;
    result.m[1][2] = forward.y;
    result.m[2][2] = forward.z;
    result.m[3][2] = -RenderDot(forward, eye);

    result.m[0][3] = 0.0f;
    result.m[1][3] = 0.0f;
    result.m[2][3] = 0.0f;
    result.m[3][3] = 1.0f;
    return result;
}

inline RenderMatrix4 RenderCreatePerspectiveMatrix(
    float fovDegrees,
    float aspectRatio,
    float nearPlane,
    float farPlane)
{
    const float clampedAspect = (aspectRatio > 0.000001f) ? aspectRatio : 1.0f;
    const float fovRadians = fovDegrees * 0.01745329251994329577f;
    const float yScale = 1.0f / std::tan(fovRadians * 0.5f);
    const float xScale = yScale / clampedAspect;
    const float depthRange = farPlane - nearPlane;

    RenderMatrix4 result = {};
    result.m[0][0] = xScale;
    result.m[1][1] = yScale;
    result.m[2][2] = farPlane / depthRange;
    result.m[2][3] = 1.0f;
    result.m[3][2] = (-nearPlane * farPlane) / depthRange;
    result.m[3][3] = 0.0f;
    return result;
}

struct RenderTransform
{
    RenderVector3 translation = RenderVector3(0.0f, 0.0f, 0.0f);
    RenderVector3 rotationRadians = RenderVector3(0.0f, 0.0f, 0.0f);
    RenderVector3 scale = RenderVector3(1.0f, 1.0f, 1.0f);

    RenderMatrix4 ToMatrix() const
    {
        const RenderMatrix4 scaleMatrix = RenderCreateScaleMatrix(scale);
        const RenderMatrix4 rotationX = RenderCreateRotationXMatrix(rotationRadians.x);
        const RenderMatrix4 rotationY = RenderCreateRotationYMatrix(rotationRadians.y);
        const RenderMatrix4 rotationZ = RenderCreateRotationZMatrix(rotationRadians.z);
        const RenderMatrix4 rotationMatrix = RenderMultiply(RenderMultiply(rotationX, rotationY), rotationZ);
        const RenderMatrix4 translationMatrix = RenderCreateTranslationMatrix(translation);
        return RenderMultiply(RenderMultiply(scaleMatrix, rotationMatrix), translationMatrix);
    }
};
