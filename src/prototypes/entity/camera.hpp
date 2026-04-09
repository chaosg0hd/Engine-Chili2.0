#pragma once

#include "../math/math.hpp"

struct CameraPrototype
{
    Vector3 position = Vector3(0.0f, 0.0f, -5.0f);
    Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
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
