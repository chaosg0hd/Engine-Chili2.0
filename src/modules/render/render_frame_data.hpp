#pragma once

#include "math/render_math.hpp"

#include <cstdint>
#include <string>
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
    Overlay2D,
    ScreenPatch,
    ScreenHexPatch
};

enum class RenderBuiltInMeshKind : unsigned char
{
    None = 0,
    Triangle,
    Diamond,
    Cube,
    Quad,
    Octahedron,
    Hex
};

struct RenderCameraData
{
    RenderVector3 position = RenderVector3(0.0f, 0.0f, -5.0f);
    RenderVector3 target = RenderVector3(0.0f, 0.0f, 0.0f);
    RenderVector3 up = RenderVector3(0.0f, 1.0f, 0.0f);
    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float exposure = 1.0f;
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
    std::string albedoAssetId;
    std::uint32_t albedoTextureHandle = 0U;
    std::uint32_t reflectionColor = 0xFFFFFFFFu;
    float reflectivity = 0.0f;
    float roughness = 1.0f;
    float ambientStrength = 0.18f;
    float diffuseStrength = 1.0f;
    float specularStrength = 0.35f;
    float specularPower = 24.0f;
};

struct RenderObjectData
{
    RenderTransform transform;
    RenderMeshData mesh;
    RenderMaterialData material;
};

struct RenderScreenPatchData
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    float halfWidth = 0.1f;
    float halfHeight = 0.1f;
    float rotationRadians = 0.0f;
    std::uint32_t color = 0xFFFFFFFFu;
};

struct RenderItemData
{
    RenderItemDataKind kind = RenderItemDataKind::Unknown;
    RenderObjectData object3D;
    RenderScreenPatchData screenPatch;
};

struct RenderSceneLightData
{
    RenderVector3 position = RenderVector3(0.0f, 0.0f, 0.0f);
    std::uint32_t color = 0xFFFFFFFFu;
    float intensity = 1.0f;
    float range = 10.0f;
    bool enabled = true;
};

struct RenderLightRayData
{
    RenderVector3 origin = RenderVector3(0.0f, 0.0f, 0.0f);
    RenderVector3 direction = RenderVector3(0.0f, -1.0f, 0.0f);
    std::uint32_t color = 0xFFFFFFFFu;
    float intensity = 1.0f;
    std::uint32_t rayCount = 1U;
    std::uint32_t maxBounceCount = 0U;
    std::uint32_t randomSeed = 0U;
    float spreadAngleRadians = 0.0f;
    float maxDistance = 100.0f;
    bool enabled = true;
};

struct RenderViewData
{
    RenderViewDataKind kind = RenderViewDataKind::Unknown;
    RenderCameraData camera;
    std::vector<RenderSceneLightData> lights;
    std::vector<RenderLightRayData> lightRayEmitters;
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
