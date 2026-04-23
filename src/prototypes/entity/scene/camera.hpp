#pragma once

#include "../../math/math.hpp"

#include <cmath>

struct QuaternionPrototype
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    static QuaternionPrototype Identity()
    {
        return QuaternionPrototype{};
    }
};

inline Vector3 RotateVector(const QuaternionPrototype& quaternion, const Vector3& value)
{
    const Vector3 quaternionVector(quaternion.x, quaternion.y, quaternion.z);
    const Vector3 uv = Cross(quaternionVector, value);
    const Vector3 uuv = Cross(quaternionVector, uv);
    return value + ((uv * quaternion.w) + uuv) * 2.0f;
}

enum class CameraProjectionModePrototype : unsigned char
{
    Perspective = 0,
    Orthographic
};

enum class CameraGateFitModePrototype : unsigned char
{
    None = 0,
    Fill,
    Overscan,
    Horizontal,
    Vertical
};

enum class CameraFocusModePrototype : unsigned char
{
    ManualDistance = 0,
    TargetBased,
    Auto
};

enum class CameraExposureModePrototype : unsigned char
{
    Manual = 0,
    Auto
};

enum class CameraPurposePrototype : unsigned char
{
    Gameplay = 0,
    Cinematic,
    Debug,
    Preview,
    ShadowHelper
};

struct CameraPosePrototype
{
    Vector3 position = Vector3(0.0f, 0.0f, -5.0f);
    Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 worldUp = Vector3(0.0f, 1.0f, 0.0f);
    bool useTarget = true;
    QuaternionPrototype orientation = QuaternionPrototype::Identity();
};

struct CameraProjectionPrototype
{
    CameraProjectionModePrototype mode = CameraProjectionModePrototype::Perspective;
    float fieldOfViewDegrees = 60.0f;
    float focalLengthMm = 50.0f;
    float sensorWidthMm = 36.0f;
    float sensorHeightMm = 24.0f;
    float aspectRatioOverride = 0.0f;
    CameraGateFitModePrototype gateFitMode = CameraGateFitModePrototype::None;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float orthographicHeight = 10.0f;
    bool usePhysicalLensModel = false;
};

struct CameraFocusPrototype
{
    CameraFocusModePrototype mode = CameraFocusModePrototype::ManualDistance;
    float focusDistance = 5.0f;
    Vector3 focusTarget = Vector3(0.0f, 0.0f, 0.0f);
};

struct CameraLensPrototype
{
    float apertureFStop = 2.8f;
    float shutterSpeedSeconds = 1.0f / 60.0f;
    float iso = 100.0f;
    int apertureBladeCount = 6;
    float anamorphicRatio = 1.0f;
};

struct CameraExposurePrototype
{
    CameraExposureModePrototype mode = CameraExposureModePrototype::Manual;
    float exposure = 1.0f;
    float exposureCompensation = 0.0f;
};

struct CameraPrototype
{
    CameraPosePrototype pose;
    CameraProjectionPrototype projection;
    CameraFocusPrototype focus;
    CameraLensPrototype lens;
    CameraExposurePrototype exposure;
    CameraPurposePrototype purpose = CameraPurposePrototype::Gameplay;
    bool enabled = true;

    Vector3 GetWorldPosition() const
    {
        return pose.position;
    }

    Vector3 GetTargetPosition() const
    {
        return pose.useTarget ? pose.target : (pose.position + GetForward());
    }

    Vector3 GetForward() const
    {
        if (pose.useTarget)
        {
            const Vector3 forward = Normalize(pose.target - pose.position);
            return (Length(forward) > 0.000001f) ? forward : Vector3(0.0f, 0.0f, 1.0f);
        }

        const Vector3 rotatedForward = Normalize(RotateVector(pose.orientation, Vector3(0.0f, 0.0f, 1.0f)));
        return (Length(rotatedForward) > 0.000001f) ? rotatedForward : Vector3(0.0f, 0.0f, 1.0f);
    }

    Vector3 GetRight() const
    {
        const Vector3 right = Normalize(Cross(pose.worldUp, GetForward()));
        return (Length(right) > 0.000001f) ? right : Vector3(1.0f, 0.0f, 0.0f);
    }

    Vector3 GetUpDirection() const
    {
        const Vector3 cameraUp = Normalize(Cross(GetForward(), GetRight()));
        return (Length(cameraUp) > 0.000001f) ? cameraUp : Vector3(0.0f, 1.0f, 0.0f);
    }

    float GetFieldOfViewDegrees() const
    {
        return projection.fieldOfViewDegrees;
    }

    float GetNearPlane() const
    {
        return projection.nearPlane;
    }

    float GetFarPlane() const
    {
        return projection.farPlane;
    }

    float GetExposureScalar() const
    {
        return exposure.exposure * std::pow(2.0f, exposure.exposureCompensation);
    }

    void LookAt(const Vector3& focusPoint)
    {
        pose.target = focusPoint;
        pose.useTarget = true;
        focus.mode = CameraFocusModePrototype::TargetBased;
        focus.focusTarget = focusPoint;
        focus.focusDistance = Length(focusPoint - pose.position);
    }

    void SetPerspective(float inFovDegrees, float inNearPlane, float inFarPlane)
    {
        projection.mode = CameraProjectionModePrototype::Perspective;
        projection.fieldOfViewDegrees = inFovDegrees;
        projection.nearPlane = inNearPlane;
        projection.farPlane = inFarPlane;
    }

    void Move(const Vector3& delta)
    {
        pose.position = pose.position + delta;
        if (pose.useTarget)
        {
            pose.target = pose.target + delta;
        }
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
        Vector3 offset = pose.position - focusPoint;
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
        pose.position = focusPoint + Vector3(
            std::sin(nextYaw) * cosPitch * radius,
            std::sin(currentPitch) * radius,
            std::cos(nextYaw) * cosPitch * radius);
        LookAt(focusPoint);
    }
};

inline Matrix4 BuildCameraViewMatrix(const CameraPrototype& camera)
{
    return CreateLookAtMatrix(
        camera.GetWorldPosition(),
        camera.GetTargetPosition(),
        camera.pose.worldUp);
}

inline Matrix4 BuildCameraProjectionMatrix(const CameraPrototype& camera, float aspectRatio)
{
    const float resolvedAspectRatio =
        camera.projection.aspectRatioOverride > 0.000001f
            ? camera.projection.aspectRatioOverride
            : aspectRatio;

    if (camera.projection.mode == CameraProjectionModePrototype::Orthographic)
    {
        return CreateOrthographicMatrix(
            camera.projection.orthographicHeight,
            resolvedAspectRatio,
            camera.projection.nearPlane,
            camera.projection.farPlane);
    }

    return CreatePerspectiveMatrix(
        camera.projection.fieldOfViewDegrees,
        resolvedAspectRatio,
        camera.projection.nearPlane,
        camera.projection.farPlane);
}
