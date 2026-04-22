#include "sandbox_scene_presets.hpp"

#include "../entity/object/mesh.hpp"
#include "../entity/scene/infinite_plane.hpp"
#include "../presentation/item.hpp"
#include "../presentation/pass.hpp"
#include "../presentation/view.hpp"

#include <cmath>
#include <cstdint>
#include <utility>

namespace
{
void AddObject(
    ViewPrototype& view,
    BuiltInMeshKind meshKind,
    const Vector3& translation,
    const Vector3& scale,
    const Vector3& rotation,
    std::uint32_t color)
{
    ItemPrototype item;
    item.kind = ItemKind::Object3D;
    item.object3D.transform.translation = translation;
    item.object3D.transform.scale = scale;
    item.object3D.transform.rotationRadians = rotation;
    MeshPrototype& mesh = item.object3D.GetPrimaryMesh();
    mesh.builtInKind = meshKind;
    mesh.material.baseLayer.albedo = ColorPrototype::FromArgb(color);
    view.items.push_back(item);
}

void ApplyMaterialPrototype(
    MeshPrototype& mesh,
    const MaterialPrototype* materialPrototype,
    const MaterialPrototype& fallback)
{
    mesh.material = materialPrototype ? *materialPrototype : fallback;
}

void AddLightingLabObject(
    ViewPrototype& view,
    BuiltInMeshKind meshKind,
    const Vector3& translation,
    const Vector3& scale,
    const Vector3& rotation,
    const MaterialPrototype* materialPrototype,
    const MaterialPrototype& fallback)
{
    AddObject(view, meshKind, translation, scale, rotation, 0xFFFFFFFFu);
    ApplyMaterialPrototype(view.items.back().object3D.GetPrimaryMesh(), materialPrototype, fallback);
}

ViewPrototype BuildDefaultSceneView(const SandboxScenePresetOptionsPrototype& options)
{
    ViewPrototype view;
    view.kind = ViewKind::Scene3D;
    view.camera = CameraPrototype{};

    const float orbitAngle = options.cameraOrbitEnabled
        ? static_cast<float>(options.totalTime * 0.35)
        : 0.0f;
    view.camera.position = Vector3(
        std::sin(orbitAngle) * 1.25f,
        0.55f + (std::sin(orbitAngle * 0.45f) * 0.12f),
        -10.2f + (std::cos(orbitAngle) * 0.4f));
    view.camera.target = Vector3(0.0f, 0.35f, 3.4f);
    view.camera.fovDegrees = 62.0f;
    view.camera.nearPlane = 0.1f;
    view.camera.farPlane = 100.0f;
    return view;
}

FramePrototype BuildVisibilityRoomFrame(const SandboxScenePresetOptionsPrototype& options)
{
    ViewPrototype view = BuildDefaultSceneView(options);
    const float rotationTime = options.rotationPaused ? 0.0f : static_cast<float>(options.totalTime);
    view.items.reserve(8);

    AddObject(view, BuiltInMeshKind::Quad, Vector3(0.0f, -2.25f, 3.6f), Vector3(7.2f, 0.12f, 7.2f), Vector3(0.0f, 0.0f, 0.0f), 0xFF2E4057u);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(0.0f, 3.15f, 3.6f), Vector3(7.2f, 0.12f, 7.2f), Vector3(0.0f, 0.0f, 0.0f), 0xFF3A506Bu);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(-3.6f, 0.45f, 3.6f), Vector3(0.12f, 5.4f, 7.2f), Vector3(0.0f, 0.0f, 0.0f), 0xFF3F5D5Bu);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(3.6f, 0.45f, 3.6f), Vector3(0.12f, 5.4f, 7.2f), Vector3(0.0f, 0.0f, 0.0f), 0xFF3F5D5Bu);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(0.0f, 0.45f, 7.2f), Vector3(7.2f, 5.4f, 0.12f), Vector3(0.0f, 0.0f, 0.0f), 0xFF34495Eu);
    AddObject(view, BuiltInMeshKind::Octahedron, Vector3(0.0f, -0.45f, 3.2f), Vector3(1.35f, 1.35f, 1.35f), Vector3(rotationTime * 0.55f, rotationTime * 0.35f, rotationTime * 0.25f), 0xFFD9C27Au);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(0.0f, 2.72f, 3.2f), Vector3(0.55f, 0.55f, 0.55f), Vector3(0.0f, rotationTime * 0.2f, 0.0f), 0xFFFFF3B0u);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(0.0f, 2.45f, 3.2f), Vector3(0.18f, 0.8f, 0.18f), Vector3(0.0f, rotationTime * 0.2f, 0.0f), 0xFF8D99AEu);

    PassPrototype pass;
    pass.kind = PassKind::Scene;
    pass.views.push_back(std::move(view));

    FramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}

FramePrototype BuildPrototypeGridFrame(const SandboxScenePresetOptionsPrototype& options)
{
    ViewPrototype view = BuildDefaultSceneView(options);
    const float rotationTime = options.rotationPaused ? 0.0f : static_cast<float>(options.totalTime);
    view.camera.target = Vector3(0.0f, 0.25f, 5.4f);
    view.items.reserve(10);

    AddObject(view, BuiltInMeshKind::Quad, Vector3(0.0f, -1.95f, 5.4f), Vector3(10.0f, 0.08f, 10.0f), Vector3(0.0f, 0.0f, 0.0f), 0xFF243447u);

    const BuiltInMeshKind meshes[] =
    {
        BuiltInMeshKind::Triangle,
        BuiltInMeshKind::Diamond,
        BuiltInMeshKind::Quad,
        BuiltInMeshKind::Cube,
        BuiltInMeshKind::Octahedron
    };

    const std::uint32_t colors[] =
    {
        0xFFE76F51u,
        0xFFF4A261u,
        0xFFE9C46Au,
        0xFF2A9D8Fu,
        0xFF4D908Eu
    };

    for (int index = 0; index < 5; ++index)
    {
        const float x = static_cast<float>(index - 2) * 1.85f;
        AddObject(
            view,
            meshes[index],
            Vector3(x, -0.2f, 4.6f),
            Vector3(0.9f, 0.9f, 0.9f),
            Vector3(rotationTime * 0.35f, rotationTime * (0.20f + (0.08f * static_cast<float>(index))), 0.0f),
            colors[index]);

        AddObject(
            view,
            meshes[index],
            Vector3(x, 1.55f, 6.4f),
            Vector3(0.65f, 0.65f, 0.65f),
            Vector3(0.0f, -rotationTime * (0.18f + (0.05f * static_cast<float>(index))), 0.0f),
            colors[4 - index]);
    }

    PassPrototype pass;
    pass.kind = PassKind::Scene;
    pass.views.push_back(std::move(view));

    FramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}

FramePrototype BuildCameraControlLabFrame(const SandboxScenePresetOptionsPrototype& options)
{
    ViewPrototype view;
    view.kind = ViewKind::Scene3D;
    view.camera = options.cameraControlCamera;
    const float rotationTime = options.rotationPaused ? 0.0f : static_cast<float>(options.totalTime);
    view.items.reserve(11);

    ItemPrototype planeItem;
    planeItem.kind = ItemKind::InfinitePlane;
    planeItem.infinitePlane.origin = Vector3(0.0f, -2.0f, 0.0f);
    planeItem.infinitePlane.baseColor = 0xFF16202Bu;
    planeItem.infinitePlane.minorLineColor = 0xFF223140u;
    planeItem.infinitePlane.majorLineColor = 0xFF3F5F7Du;
    planeItem.infinitePlane.minorGridSpacing = 1.0f;
    planeItem.infinitePlane.majorGridSpacing = 5.0f;
    planeItem.infinitePlane.lineThickness = 0.03f;
    planeItem.infinitePlane.extent = 20.0f;
    view.items.push_back(planeItem);

    AddObject(view, BuiltInMeshKind::Cube, Vector3(0.0f, -0.8f, 3.0f), Vector3(1.1f, 1.1f, 1.1f), Vector3(rotationTime * 0.20f, rotationTime * 0.35f, 0.0f), 0xFFE76F51u);
    AddObject(view, BuiltInMeshKind::Cube, Vector3(-4.0f, -0.8f, 7.0f), Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, rotationTime * 0.15f, 0.0f), 0xFF4D908Eu);
    AddObject(view, BuiltInMeshKind::Cube, Vector3(4.0f, -0.8f, 7.0f), Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, -rotationTime * 0.15f, 0.0f), 0xFF577590u);
    AddObject(view, BuiltInMeshKind::Octahedron, Vector3(0.0f, 0.9f, 11.0f), Vector3(0.9f, 0.9f, 0.9f), Vector3(rotationTime * 0.30f, rotationTime * 0.20f, rotationTime * 0.10f), 0xFFF4A261u);
    AddObject(view, BuiltInMeshKind::Triangle, Vector3(0.0f, 2.1f, 15.0f), Vector3(0.95f, 0.95f, 0.95f), Vector3(0.0f, rotationTime * 0.25f, 0.0f), 0xFFF94144u);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(0.0f, 4.8f, 7.0f), Vector3(16.0f, 0.05f, 22.0f), Vector3(0.0f, 0.0f, 0.0f), 0xFF253445u);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(-8.0f, 1.4f, 7.0f), Vector3(0.05f, 6.8f, 22.0f), Vector3(0.0f, 0.0f, 0.0f), 0xFF31465Au);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(8.0f, 1.4f, 7.0f), Vector3(0.05f, 6.8f, 22.0f), Vector3(0.0f, 0.0f, 0.0f), 0xFF31465Au);
    AddObject(view, BuiltInMeshKind::Quad, Vector3(0.0f, 1.4f, 18.0f), Vector3(16.0f, 6.8f, 0.05f), Vector3(0.0f, 0.0f, 0.0f), 0xFF2A3C4Eu);
    AddObject(view, BuiltInMeshKind::Diamond, Vector3(0.0f, -1.2f, 7.0f), Vector3(0.55f, 0.55f, 0.55f), Vector3(0.0f, rotationTime * 0.40f, 0.0f), 0xFFFFFF99u);

    PassPrototype pass;
    pass.kind = PassKind::Scene;
    pass.views.push_back(std::move(view));

    FramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}

FramePrototype BuildLightingLabFrame(const SandboxScenePresetOptionsPrototype& options)
{
    constexpr float roomCenterZ = 4.5f;
    constexpr float roomClearWidthMeters = 4.0f;
    constexpr float roomClearHeightMeters = 4.0f;
    constexpr float wallThicknessMeters = 0.08f;
    constexpr float floorThicknessMeters = 0.08f;
    constexpr float floorTopY = -0.5f;
    constexpr float floorCenterY = floorTopY - (floorThicknessMeters * 0.5f);
    constexpr float wallCenterY = floorTopY + (roomClearHeightMeters * 0.5f);
    constexpr float roomHalfWidth = roomClearWidthMeters * 0.5f;
    constexpr float roomHalfDepth = roomClearWidthMeters * 0.5f;
    constexpr float roomOuterWidth = roomClearWidthMeters + (wallThicknessMeters * 2.0f);
    constexpr float roomOuterDepth = roomClearWidthMeters + (wallThicknessMeters * 2.0f);
    constexpr float leftWallCenterX = -(roomHalfWidth + (wallThicknessMeters * 0.5f));
    constexpr float rightWallCenterX = roomHalfWidth + (wallThicknessMeters * 0.5f);
    constexpr float backWallCenterZ = roomCenterZ + roomHalfDepth + (wallThicknessMeters * 0.5f);

    ViewPrototype view;
    view.kind = ViewKind::Scene3D;
    if (options.cameraOverrideEnabled)
    {
        view.camera = options.cameraControlCamera;
    }
    else
    {
        view.camera = CameraPrototype{};

        const float orbitAngle = options.cameraOrbitEnabled
            ? static_cast<float>(options.totalTime * 0.32)
            : 0.0f;
        view.camera.position = Vector3(
            std::sin(orbitAngle) * 3.2f,
            1.7f + (std::sin(orbitAngle * 0.7f) * 0.25f),
            0.6f + (std::cos(orbitAngle) * 0.9f));
        view.camera.target = Vector3(0.0f, 0.15f, roomCenterZ);
        view.camera.fovDegrees = 52.0f;
        view.camera.nearPlane = 0.1f;
        view.camera.farPlane = 200.0f;
    }

    const float rotationTime = options.rotationPaused ? 0.0f : static_cast<float>(options.totalTime);
    view.items.reserve(7);
    view.lights.reserve(1);
    view.lights.push_back(options.primarySceneLight);

    MaterialPrototype defaultRoomMaterial;
    defaultRoomMaterial.baseLayer.albedo = ColorPrototype(1.0f, 1.0f, 1.0f, 1.0f);
    defaultRoomMaterial.baseLayer.roughness = 0.82f;
    defaultRoomMaterial.reflectivity = 0.0f;
    defaultRoomMaterial.reflectionColor = defaultRoomMaterial.baseLayer.albedo;
    defaultRoomMaterial.brdf.ambientStrength = 0.07f;
    defaultRoomMaterial.brdf.diffuseStrength = 0.94f;
    defaultRoomMaterial.brdf.specularStrength = 0.06f;
    defaultRoomMaterial.brdf.specularPower = 10.0f;

    MaterialPrototype defaultFloorMaterial = defaultRoomMaterial;
    defaultFloorMaterial.baseLayer.roughness = 0.86f;
    defaultFloorMaterial.brdf.ambientStrength = 0.06f;

    MaterialPrototype defaultCubeMaterial = defaultRoomMaterial;
    defaultCubeMaterial.baseLayer.roughness = 0.34f;
    defaultCubeMaterial.brdf.ambientStrength = 0.08f;
    defaultCubeMaterial.brdf.diffuseStrength = 0.90f;
    defaultCubeMaterial.brdf.specularStrength = 0.26f;
    defaultCubeMaterial.brdf.specularPower = 22.0f;

    MaterialPrototype defaultEmitterMaterial;
    defaultEmitterMaterial.baseLayer.albedo = ColorPrototype::FromArgb(0xFFFFF1D0u);
    defaultEmitterMaterial.reflectivity = 0.0f;
    defaultEmitterMaterial.reflectionColor = defaultEmitterMaterial.baseLayer.albedo;
    defaultEmitterMaterial.baseLayer.roughness = 0.12f;
    defaultEmitterMaterial.brdf.ambientStrength = 0.40f;
    defaultEmitterMaterial.brdf.diffuseStrength = 0.45f;
    defaultEmitterMaterial.brdf.specularStrength = 0.18f;
    defaultEmitterMaterial.brdf.specularPower = 16.0f;

    AddLightingLabObject(
        view,
        BuiltInMeshKind::Cube,
        Vector3(0.0f, floorCenterY, roomCenterZ),
        Vector3(roomOuterWidth, floorThicknessMeters, roomOuterDepth),
        Vector3(0.0f, 0.0f, 0.0f),
        options.floorMaterial,
        defaultFloorMaterial);

    AddLightingLabObject(
        view,
        BuiltInMeshKind::Cube,
        Vector3(leftWallCenterX, wallCenterY, roomCenterZ),
        Vector3(wallThicknessMeters, roomClearHeightMeters, roomOuterDepth),
        Vector3(0.0f, 0.0f, 0.0f),
        options.roomMaterial,
        defaultRoomMaterial);

    AddLightingLabObject(
        view,
        BuiltInMeshKind::Cube,
        Vector3(rightWallCenterX, wallCenterY, roomCenterZ),
        Vector3(wallThicknessMeters, roomClearHeightMeters, roomOuterDepth),
        Vector3(0.0f, 0.0f, 0.0f),
        options.roomMaterial,
        defaultRoomMaterial);

    AddLightingLabObject(
        view,
        BuiltInMeshKind::Cube,
        Vector3(0.0f, wallCenterY, backWallCenterZ),
        Vector3(roomClearWidthMeters, roomClearHeightMeters, wallThicknessMeters),
        Vector3(0.0f, 0.0f, 0.0f),
        options.roomMaterial,
        defaultRoomMaterial);

    AddLightingLabObject(
        view,
        BuiltInMeshKind::Cube,
        Vector3(0.0f, 0.5f, roomCenterZ),
        Vector3(1.0f, 1.0f, 1.0f),
        Vector3(rotationTime * 0.35f, rotationTime * 0.55f, rotationTime * 0.12f),
        options.cubeMaterial,
        defaultCubeMaterial);

    AddObject(
        view,
        BuiltInMeshKind::Octahedron,
        Vector3(0.0f, 2.4f, roomCenterZ),
        Vector3(0.16f, 0.16f, 0.16f),
        Vector3(0.0f, rotationTime * 0.8f, 0.0f),
        0xFFFFF1D0u);
    ApplyMaterialPrototype(
        view.items.back().object3D.GetPrimaryMesh(),
        options.emitterMaterial,
        defaultEmitterMaterial);

    PassPrototype pass;
    pass.kind = PassKind::Scene;
    pass.views.push_back(std::move(view));

    FramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}
}

FramePrototype SandboxScenePresetCompiler::BuildFrame(
    SandboxPresetScenePrototype scene,
    const SandboxScenePresetOptionsPrototype& options)
{
    switch (scene)
    {
    case SandboxPresetScenePrototype::LightingLab:
        return BuildLightingLabFrame(options);
    case SandboxPresetScenePrototype::PrototypeGrid:
        return BuildPrototypeGridFrame(options);
    case SandboxPresetScenePrototype::CameraControlLab:
        return BuildCameraControlLabFrame(options);
    case SandboxPresetScenePrototype::VisibilityRoom:
    default:
        return BuildVisibilityRoomFrame(options);
    }
}

unsigned int SandboxScenePresetCompiler::CountFrameItems(const FramePrototype& frame)
{
    unsigned int itemCount = 0U;

    for (const PassPrototype& pass : frame.passes)
    {
        for (const ViewPrototype& view : pass.views)
        {
            itemCount += static_cast<unsigned int>(view.items.size());
        }
    }

    return itemCount;
}
