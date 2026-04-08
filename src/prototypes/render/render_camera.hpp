#pragma once

#include "render_math.hpp"

struct RenderCamera
{
    RenderVector3 position = RenderVector3(0.0f, 0.0f, -5.0f);
    RenderVector3 target = RenderVector3(0.0f, 0.0f, 0.0f);
    RenderVector3 up = RenderVector3(0.0f, 1.0f, 0.0f);
    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

inline RenderMatrix4 BuildRenderCameraViewMatrix(const RenderCamera& camera)
{
    return CreateLookAtMatrix(camera.position, camera.target, camera.up);
}

inline RenderMatrix4 BuildRenderCameraProjectionMatrix(const RenderCamera& camera,
                                                       float aspectRatio)
{
    return CreatePerspectiveMatrix(camera.fovDegrees,
                                   aspectRatio,
                                   camera.nearPlane,
                                   camera.farPlane);
}
