#pragma once

#include "../../math/math.hpp"

struct CameraPrototype
{
    Vector3 position = Vector3(0.0f, 0.0f, -5.0f);
    Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    Vector3 GetForward() const
    {
        const Vector3 forward = Normalize(target - position);
        return (Length(forward) > 0.000001f) ? forward : Vector3(0.0f, 0.0f, 1.0f);
    }

    Vector3 GetRight() const
    {
        const Vector3 right = Normalize(Cross(up, GetForward()));
        return (Length(right) > 0.000001f) ? right : Vector3(1.0f, 0.0f, 0.0f);
    }

    Vector3 GetUpDirection() const
    {
        const Vector3 cameraUp = Normalize(Cross(GetForward(), GetRight()));
        return (Length(cameraUp) > 0.000001f) ? cameraUp : Vector3(0.0f, 1.0f, 0.0f);
    }

    void LookAt(const Vector3& focusPoint)
    {
        target = focusPoint;
    }

    void SetPerspective(float inFovDegrees, float inNearPlane, float inFarPlane)
    {
        fovDegrees = inFovDegrees;
        nearPlane = inNearPlane;
        farPlane = inFarPlane;
    }

    void Move(const Vector3& delta)
    {
        position = position + delta;
        target = target + delta;
    }

    void MoveForward(float distance)
    {
        Move(GetForward() * distance);
    }

    void MoveRight(float distance)
    {
        Move(GetRight() * distance);
    }

    void MoveUp(float distance)
    {
        Move(GetUpDirection() * distance);
    }

    void TranslateLocal(float forwardDistance, float rightDistance, float upDistance)
    {
        Move(
            (GetForward() * forwardDistance) +
            (GetRight() * rightDistance) +
            (GetUpDirection() * upDistance));
    }

    void OrbitAround(const Vector3& focusPoint, float yawDeltaRadians, float pitchDeltaRadians)
    {
        Vector3 offset = position - focusPoint;
        float radius = Length(offset);
        if (radius <= 0.000001f)
        {
            radius = 1.0f;
            offset = Vector3(0.0f, 0.0f, -radius);
        }

        const float currentYaw = std::atan2(offset.x, offset.z);
        const float horizontalLength = std::sqrt((offset.x * offset.x) + (offset.z * offset.z));
        float currentPitch = std::atan2(offset.y, horizontalLength);
        currentPitch += pitchDeltaRadians;

        const float pitchLimit = 1.55334306f;
        if (currentPitch > pitchLimit)
        {
            currentPitch = pitchLimit;
        }
        else if (currentPitch < -pitchLimit)
        {
            currentPitch = -pitchLimit;
        }

        const float nextYaw = currentYaw + yawDeltaRadians;
        const float cosPitch = std::cos(currentPitch);
        position = focusPoint + Vector3(
            std::sin(nextYaw) * cosPitch * radius,
            std::sin(currentPitch) * radius,
            std::cos(nextYaw) * cosPitch * radius);
        target = focusPoint;
    }
};

inline Matrix4 BuildCameraViewMatrix(const CameraPrototype& camera)
{
    return CreateLookAtMatrix(camera.position, camera.target, camera.up);
}

inline Matrix4 BuildCameraProjectionMatrix(const CameraPrototype& camera,
                                           float aspectRatio)
{
    return CreatePerspectiveMatrix(camera.fovDegrees,
                                   aspectRatio,
                                   camera.nearPlane,
                                   camera.farPlane);
}
