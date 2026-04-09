#pragma once

#include <cmath>

struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vector3() = default;
    constexpr Vector3(float inX, float inY, float inZ)
        : x(inX), y(inY), z(inZ)
    {
    }

    constexpr Vector3 operator+(const Vector3& other) const
    {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    constexpr Vector3 operator-(const Vector3& other) const
    {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    constexpr Vector3 operator*(float scalar) const
    {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
};

inline float Dot(const Vector3& a, const Vector3& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

inline Vector3 Cross(const Vector3& a, const Vector3& b)
{
    return Vector3(
        (a.y * b.z) - (a.z * b.y),
        (a.z * b.x) - (a.x * b.z),
        (a.x * b.y) - (a.y * b.x));
}

inline float Length(const Vector3& value)
{
    return std::sqrt(Dot(value, value));
}

inline Vector3 Normalize(const Vector3& value)
{
    const float length = Length(value);
    if (length <= 0.000001f)
    {
        return Vector3{};
    }

    return value * (1.0f / length);
}

struct Matrix4
{
    float m[4][4] =
    {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };

    static Matrix4 Identity()
    {
        return Matrix4{};
    }
};

inline Matrix4 Multiply(const Matrix4& a, const Matrix4& b)
{
    Matrix4 result = {};

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

inline Matrix4 CreateTranslationMatrix(const Vector3& translation)
{
    Matrix4 result = Matrix4::Identity();
    result.m[3][0] = translation.x;
    result.m[3][1] = translation.y;
    result.m[3][2] = translation.z;
    return result;
}

inline Matrix4 CreateScaleMatrix(const Vector3& scale)
{
    Matrix4 result = Matrix4::Identity();
    result.m[0][0] = scale.x;
    result.m[1][1] = scale.y;
    result.m[2][2] = scale.z;
    return result;
}

inline Matrix4 CreateRotationXMatrix(float radians)
{
    Matrix4 result = Matrix4::Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    result.m[1][1] = c;
    result.m[1][2] = s;
    result.m[2][1] = -s;
    result.m[2][2] = c;
    return result;
}

inline Matrix4 CreateRotationYMatrix(float radians)
{
    Matrix4 result = Matrix4::Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    result.m[0][0] = c;
    result.m[0][2] = -s;
    result.m[2][0] = s;
    result.m[2][2] = c;
    return result;
}

inline Matrix4 CreateRotationZMatrix(float radians)
{
    Matrix4 result = Matrix4::Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    result.m[0][0] = c;
    result.m[0][1] = s;
    result.m[1][0] = -s;
    result.m[1][1] = c;
    return result;
}

inline Matrix4 CreateLookAtMatrix(
    const Vector3& eye,
    const Vector3& target,
    const Vector3& up)
{
    const Vector3 forward = Normalize(target - eye);
    const Vector3 right = Normalize(Cross(up, forward));
    const Vector3 cameraUp = Cross(forward, right);

    Matrix4 result = {};
    result.m[0][0] = right.x;
    result.m[1][0] = right.y;
    result.m[2][0] = right.z;
    result.m[3][0] = -Dot(right, eye);

    result.m[0][1] = cameraUp.x;
    result.m[1][1] = cameraUp.y;
    result.m[2][1] = cameraUp.z;
    result.m[3][1] = -Dot(cameraUp, eye);

    result.m[0][2] = forward.x;
    result.m[1][2] = forward.y;
    result.m[2][2] = forward.z;
    result.m[3][2] = -Dot(forward, eye);

    result.m[0][3] = 0.0f;
    result.m[1][3] = 0.0f;
    result.m[2][3] = 0.0f;
    result.m[3][3] = 1.0f;
    return result;
}

inline Matrix4 CreatePerspectiveMatrix(
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

    Matrix4 result = {};
    result.m[0][0] = xScale;
    result.m[1][1] = yScale;
    result.m[2][2] = farPlane / depthRange;
    result.m[2][3] = 1.0f;
    result.m[3][2] = (-nearPlane * farPlane) / depthRange;
    result.m[3][3] = 0.0f;
    return result;
}

struct TransformPrototype
{
    Vector3 translation = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 rotationRadians = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);

    Matrix4 ToMatrix() const
    {
        const Matrix4 scaleMatrix = CreateScaleMatrix(scale);
        const Matrix4 rotationX = CreateRotationXMatrix(rotationRadians.x);
        const Matrix4 rotationY = CreateRotationYMatrix(rotationRadians.y);
        const Matrix4 rotationZ = CreateRotationZMatrix(rotationRadians.z);
        const Matrix4 rotationMatrix = Multiply(Multiply(rotationX, rotationY), rotationZ);
        const Matrix4 translationMatrix = CreateTranslationMatrix(translation);
        return Multiply(Multiply(scaleMatrix, rotationMatrix), translationMatrix);
    }
};
