#include "runtime/studio_default_scene_template.hpp"

#include <sstream>

namespace studio_runtime
{
    namespace
    {
        constexpr EntityId kOriginId = 1000;
        constexpr EntityId kAxisXId = 1010;
        constexpr EntityId kAxisYId = 1020;
        constexpr EntityId kAxisZId = 1030;
        constexpr EntityId kEditorCameraId = 2100;
        constexpr EntityId kMainCameraId = 2200;

        void SetColoredCube(
            RuntimeWorld& world,
            EntityId id,
            const std::string& name,
            const Vector3& position,
            const Vector3& scale,
            const ColorPrototype& albedo)
        {
            world.CreateEntityWithId(id, name);

            TransformPrototype transform;
            transform.translation = position;
            transform.scale = scale;
            world.SetTransform(id, transform);

            ObjectComponent object;
            object.kind = "Object";
            object.selectable = true;
            world.SetObject(id, object);

            RenderableComponent renderable;
            renderable.mesh = BuiltInMeshKind::Cube;
            renderable.visible = true;
            renderable.material.baseLayer.albedo = albedo;
            world.SetRenderable(id, renderable);
        }

        void HideTemplateOverlay(RuntimeWorld& world, EntityId id)
        {
            if (RenderableComponent* renderable = world.GetRenderable(id))
            {
                // The entity remains for validation/identity, while BuildWorldFrame
                // owns the editor reference visualization so it cannot pollute snaps.
                renderable->visible = false;
            }
        }
    }

    void ApplyDefaultSceneTemplate(RuntimeWorld& world, StudioCamera& camera)
    {
        world.Clear();

        SetColoredCube(
            world,
            kOriginId,
            "OriginMarker",
            Vector3(0.0f, 0.0f, 0.0f),
            Vector3(0.14f, 0.14f, 0.14f),
            ColorPrototype::FromBytes(230, 230, 230));
        if (RenderableComponent* renderable = world.GetRenderable(kOriginId))
        {
            renderable->mesh = BuiltInMeshKind::Octahedron;
        }
        HideTemplateOverlay(world, kOriginId);

        SetColoredCube(
            world,
            kAxisXId,
            "Axis_X",
            Vector3(1.0f, 0.0f, 0.0f),
            Vector3(2.0f, 0.03f, 0.03f),
            ColorPrototype::FromBytes(233, 86, 86));
        HideTemplateOverlay(world, kAxisXId);

        SetColoredCube(
            world,
            kAxisYId,
            "Axis_Y",
            Vector3(0.0f, 1.0f, 0.0f),
            Vector3(0.03f, 2.0f, 0.03f),
            ColorPrototype::FromBytes(112, 214, 116));
        HideTemplateOverlay(world, kAxisYId);

        SetColoredCube(
            world,
            kAxisZId,
            "Axis_Z",
            Vector3(0.0f, 0.0f, 1.0f),
            Vector3(0.03f, 0.03f, 2.0f),
            ColorPrototype::FromBytes(94, 166, 245));
        HideTemplateOverlay(world, kAxisZId);

        world.CreateEntityWithId(kEditorCameraId, "EditorCamera");
        ObjectComponent editorCameraObject;
        editorCameraObject.kind = "Camera";
        editorCameraObject.selectable = true;
        world.SetObject(kEditorCameraId, editorCameraObject);
        CameraComponent editorCamera;
        editorCamera.camera.pose.position = Vector3(-4.0f, 3.0f, -6.0f);
        editorCamera.camera.LookAt(Vector3(0.5f, 0.4f, 0.5f));
        editorCamera.camera.projection.fieldOfViewDegrees = 60.0f;
        editorCamera.camera.projection.nearPlane = 0.1f;
        editorCamera.camera.projection.farPlane = 500.0f;
        editorCamera.camera.purpose = CameraPurposePrototype::Preview;
        TransformPrototype editorCameraTransform;
        editorCameraTransform.translation = editorCamera.camera.pose.position;
        world.SetTransform(kEditorCameraId, editorCameraTransform);
        world.SetCamera(kEditorCameraId, editorCamera);

        world.CreateEntityWithId(kMainCameraId, "MainCamera");
        ObjectComponent mainCameraObject;
        mainCameraObject.kind = "Camera";
        mainCameraObject.selectable = true;
        world.SetObject(kMainCameraId, mainCameraObject);
        CameraComponent mainCamera;
        mainCamera.camera.pose.position = Vector3(4.8f, 2.2f, -5.2f);
        mainCamera.camera.LookAt(Vector3(0.75f, 0.5f, 0.75f));
        mainCamera.camera.projection.fieldOfViewDegrees = 50.0f;
        mainCamera.camera.projection.nearPlane = 0.1f;
        mainCamera.camera.projection.farPlane = 500.0f;
        mainCamera.camera.purpose = CameraPurposePrototype::Gameplay;
        TransformPrototype mainCameraTransform;
        mainCameraTransform.translation = mainCamera.camera.pose.position;
        world.SetTransform(kMainCameraId, mainCameraTransform);
        world.SetCamera(kMainCameraId, mainCamera);

        camera.GetCamera() = editorCamera.camera;
        camera.SetOrbitTarget(editorCamera.camera.pose.target);
    }

    DefaultSceneTemplatePresence ValidateDefaultSceneTemplate(const RuntimeWorld& world, const StudioCamera& camera)
    {
        DefaultSceneTemplatePresence presence;
        presence.hasOrigin = world.Contains(kOriginId);
        presence.hasAxes = world.Contains(kAxisXId) && world.Contains(kAxisYId) && world.Contains(kAxisZId);

        const CameraComponent* editorCamera = world.GetCamera(kEditorCameraId);
        const CameraComponent* mainCamera = world.GetCamera(kMainCameraId);
        const CameraPrototype& viewCamera = camera.GetCamera();
        presence.hasCamera =
            editorCamera != nullptr &&
            mainCamera != nullptr &&
            viewCamera.projection.fieldOfViewDegrees > 1.0f &&
            viewCamera.projection.farPlane > viewCamera.projection.nearPlane;
        return presence;
    }

    std::string BuildDefaultSceneTemplateReport(const DefaultSceneTemplatePresence& presence)
    {
        std::ostringstream out;
        out << "Default scene template created | "
            << "origin=" << (presence.hasOrigin ? "yes" : "no")
            << " axes=" << (presence.hasAxes ? "yes" : "no")
            << " grid=" << (presence.hasGrid ? "yes" : "no")
            << " camera=" << (presence.hasCamera ? "yes" : "no");
        return out.str();
    }
}
