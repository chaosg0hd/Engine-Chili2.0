#include "runtime/scene_runtime.hpp"

#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"
#include "prototypes/presentation/view.hpp"

#include <utility>

namespace studio_runtime
{
    void SceneRuntime::ResetToMinimalScene()
    {
        m_scene = ScenePrototype{};
        m_scene.entities.push_back(EntityPrototype{
            Vector3(0.0f, 0.0f, 3.5f),
            BuiltInMeshKind::Cube,
            "player"});
        m_scene.entities.push_back(EntityPrototype{
            Vector3(1.8f, -0.3f, 5.2f),
            BuiltInMeshKind::Octahedron,
            "accent"});

        m_camera = CameraPrototype{};
        m_camera.pose.position = Vector3(0.0f, 0.8f, -8.0f);
        m_camera.pose.target = Vector3(0.0f, 0.0f, 4.0f);
        m_camera.projection.fieldOfViewDegrees = 60.0f;
        m_camera.projection.nearPlane = 0.1f;
        m_camera.projection.farPlane = 100.0f;

        m_light = LightPrototype{};
        m_light.emitter.kind = LightEmitterKind::Point;
        m_light.emitter.position = Vector3(-1.5f, 2.6f, 1.5f);
        m_light.emitter.color = ColorPrototype::FromBytes(255, 243, 210);
        m_light.emitter.intensity = 4.8f;
        m_light.emitter.range = 22.0f;
        m_light.visibility.kind = LightVisibilityKind::RasterShadowCubemap;
        m_light.visibility.policy.enabled = false;
        m_light.visibility.policy.method = LightVisibilityMethodPrototype::None;
    }

    FramePrototype SceneRuntime::BuildFrame() const
    {
        ViewPrototype view;
        view.kind = ViewKind::Scene3D;
        view.camera = m_camera;
        view.directLights.push_back(m_light);

        ItemPrototype gridItem;
        gridItem.kind = ItemKind::Grid;
        // Keep default scene grid phase consistent with Studio world view: origin marker sits between cells.
        gridItem.grid.origin = Vector3(0.5f, -1.25f, 0.5f);
        gridItem.grid.extent = 26.0f;
        gridItem.grid.cellSize = 1.0f;
        gridItem.grid.majorLineEvery = 4;
        gridItem.grid.baseColor      = ColorPrototype::FromArgb(0xFF141A22u);
        gridItem.grid.majorLineColor = ColorPrototype::FromArgb(0xFF3A4A60u);
        gridItem.grid.minorLineColor = ColorPrototype::FromArgb(0xFF263446u);
        gridItem.grid.lineThickness = 0.012f;
        view.items.push_back(std::move(gridItem));

        for (const EntityPrototype& entity : m_scene.entities)
        {
            ItemPrototype item;
            item.kind = ItemKind::Object3D;
            item.object3D.transform.translation = entity.position;
            item.object3D.transform.scale = Vector3(1.0f, 1.0f, 1.0f);
            item.object3D.transform.rotationRadians = Vector3(0.0f, 0.0f, 0.0f);

            MeshPrototype& mesh = item.object3D.GetPrimaryMesh();
            mesh.builtInKind = entity.mesh;
            mesh.material = ResolveMaterial(entity.materialRef);
            view.items.push_back(std::move(item));
        }

        PassPrototype pass;
        pass.kind = PassKind::Scene;
        pass.views.push_back(std::move(view));

        FramePrototype frame;
        frame.passes.push_back(std::move(pass));
        return frame;
    }

    void SceneRuntime::OrbitCamera(float yawDeltaRadians, float pitchDeltaRadians)
    {
        m_camera.OrbitAround(m_camera.pose.target, yawDeltaRadians, pitchDeltaRadians);
    }

    MaterialPrototype SceneRuntime::ResolveMaterial(const std::string& materialRef) const
    {
        MaterialPrototype material;
        material.baseLayer.roughness = 0.55f;

        if (materialRef == "player")
        {
            material.baseLayer.albedo = ColorPrototype::FromBytes(86, 214, 163);
            return material;
        }

        if (materialRef == "accent")
        {
            material.baseLayer.albedo = ColorPrototype::FromBytes(244, 169, 111);
            return material;
        }

        material.baseLayer.albedo = ColorPrototype::FromBytes(210, 220, 231);
        return material;
    }
}
