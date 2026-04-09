#pragma once

#include "math/render_math.hpp"

#include <cstdint>
#include <vector>

enum class RenderPassDataKind : unsigned char
{
    Unknown = 0,
    Scene,
    Overlay,
    Composite
};

enum class RenderViewDataKind : unsigned char
{
    Unknown = 0,
    Scene3D,
    Overlay2D
};

enum class RenderItemDataKind : unsigned char
{
    Unknown = 0,
    Object3D,
    Overlay2D
};

enum class RenderBuiltInMeshKind : unsigned char
{
    None = 0,
    Triangle,
    Diamond,
    Cube,
    Quad,
    Octahedron
};

struct RenderCameraData
{
    RenderVector3 position = RenderVector3(0.0f, 0.0f, -5.0f);
    RenderVector3 target = RenderVector3(0.0f, 0.0f, 0.0f);
    RenderVector3 up = RenderVector3(0.0f, 1.0f, 0.0f);
    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

inline RenderMatrix4 BuildRenderCameraViewMatrix(const RenderCameraData& camera)
{
    return RenderCreateLookAtMatrix(camera.position, camera.target, camera.up);
}

inline RenderMatrix4 BuildRenderCameraProjectionMatrix(const RenderCameraData& camera, float aspectRatio)
{
    return RenderCreatePerspectiveMatrix(camera.fovDegrees, aspectRatio, camera.nearPlane, camera.farPlane);
}

struct RenderMeshData
{
    std::uint32_t handle = 0;
    RenderBuiltInMeshKind builtInKind = RenderBuiltInMeshKind::None;
};

struct RenderMaterialData
{
    std::uint32_t handle = 0;
    std::uint32_t baseColor = 0xFFFFFFFFu;
};

struct RenderObjectData
{
    RenderTransform transform;
    RenderMeshData mesh;
    RenderMaterialData material;
};

struct RenderItemData
{
    RenderItemDataKind kind = RenderItemDataKind::Unknown;
    RenderObjectData object3D;
};

struct RenderViewData
{
    RenderViewDataKind kind = RenderViewDataKind::Unknown;
    RenderCameraData camera;
    std::vector<RenderItemData> items;
};

struct RenderPassData
{
    RenderPassDataKind kind = RenderPassDataKind::Unknown;
    std::vector<RenderViewData> views;
};

struct RenderFrameData
{
    std::vector<RenderPassData> passes;
};
