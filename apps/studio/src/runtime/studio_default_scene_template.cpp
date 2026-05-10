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
        constexpr EntityId kSampleCubeId = 2000;
        constexpr EntityId kMainLightId = 3000;

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

        SetColoredCube(
            world,
            kSampleCubeId,
            "SampleCube",
            Vector3(0.75f, 0.5f, 0.75f),
            Vector3(1.0f, 1.0f, 1.0f),
            ColorPrototype::FromBytes(196, 206, 222));

        world.CreateEntityWithId(kMainLightId, "MainLight");
        ObjectComponent lightObject;
        lightObject.kind = "Light";
        lightObject.selectable = true;
        world.SetObject(kMainLightId, lightObject);

        TransformPrototype lightTransform;
        lightTransform.translation = Vector3(-2.4f, 3.2f, -1.8f);
        world.SetTransform(kMainLightId, lightTransform);

        LightComponent light;
        light.light.enabled = true;
        light.light.emitter.kind = LightEmitterKind::Point;
        light.light.emitter.position = lightTransform.translation;
        light.light.emitter.color = ColorPrototype::FromBytes(255, 243, 214);
        light.light.emitter.intensity = 3.6f;
        light.light.emitter.range = 22.0f;
        world.SetLight(kMainLightId, light);

        CameraPrototype& viewCamera = camera.GetCamera();
        viewCamera.pose.position = Vector3(-4.0f, 3.0f, -6.0f);
        viewCamera.pose.target = Vector3(0.5f, 0.4f, 0.5f);
        viewCamera.pose.useTarget = true;
        viewCamera.projection.fieldOfViewDegrees = 60.0f;
        viewCamera.projection.nearPlane = 0.1f;
        viewCamera.projection.farPlane = 500.0f;
        camera.SetOrbitTarget(viewCamera.pose.target);
    }

    DefaultSceneTemplatePresence ValidateDefaultSceneTemplate(const RuntimeWorld& world, const StudioCamera& camera)
    {
        DefaultSceneTemplatePresence presence;
        presence.hasOrigin = world.Contains(kOriginId);
        presence.hasSampleCube = world.Contains(kSampleCubeId);
        presence.hasLight = world.Contains(kMainLightId) && world.GetLight(kMainLightId) != nullptr;
        presence.hasAxes = world.Contains(kAxisXId) && world.Contains(kAxisYId) && world.Contains(kAxisZId);

        const CameraPrototype& viewCamera = camera.GetCamera();
        presence.hasCamera =
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
            << " sample_cube=" << (presence.hasSampleCube ? "yes" : "no")
            << " camera=" << (presence.hasCamera ? "yes" : "no")
            << " light=" << (presence.hasLight ? "yes" : "no");
        return out.str();
    }

    std::string BuildDefaultSceneTemplateSceneJson()
    {
        return
            "{\n"
            "  \"version\": 1,\n"
            "  \"entities\": [\n"
            "    {\n"
            "      \"id\": 1000,\n"
            "      \"name\": \"OriginMarker\",\n"
            "      \"values\": {\n"
            "        \"transform\": {\n"
            "          \"position\": [0.0, 0.0, 0.0],\n"
            "          \"rotation\": [0.0, 0.0, 0.0],\n"
            "          \"scale\": [0.14, 0.14, 0.14]\n"
            "        }\n"
            "      },\n"
            "      \"overrides\": {},\n"
            "      \"object\": { \"kind\": \"Object\", \"selectable\": true },\n"
            "      \"renderable\": { \"mesh\": \"builtin:octahedron\", \"visible\": false, \"albedo\": [0.9, 0.9, 0.9] }\n"
            "    },\n"
            "    {\n"
            "      \"id\": 1010,\n"
            "      \"name\": \"Axis_X\",\n"
            "      \"values\": { \"transform\": { \"position\": [1.0, 0.0, 0.0], \"rotation\": [0.0, 0.0, 0.0], \"scale\": [2.0, 0.03, 0.03] } },\n"
            "      \"overrides\": {},\n"
            "      \"object\": { \"kind\": \"Object\", \"selectable\": true },\n"
            "      \"renderable\": { \"mesh\": \"builtin:cube\", \"visible\": false, \"albedo\": [0.914, 0.337, 0.337] }\n"
            "    },\n"
            "    {\n"
            "      \"id\": 1020,\n"
            "      \"name\": \"Axis_Y\",\n"
            "      \"values\": { \"transform\": { \"position\": [0.0, 1.0, 0.0], \"rotation\": [0.0, 0.0, 0.0], \"scale\": [0.03, 2.0, 0.03] } },\n"
            "      \"overrides\": {},\n"
            "      \"object\": { \"kind\": \"Object\", \"selectable\": true },\n"
            "      \"renderable\": { \"mesh\": \"builtin:cube\", \"visible\": false, \"albedo\": [0.439, 0.839, 0.455] }\n"
            "    },\n"
            "    {\n"
            "      \"id\": 1030,\n"
            "      \"name\": \"Axis_Z\",\n"
            "      \"values\": { \"transform\": { \"position\": [0.0, 0.0, 1.0], \"rotation\": [0.0, 0.0, 0.0], \"scale\": [0.03, 0.03, 2.0] } },\n"
            "      \"overrides\": {},\n"
            "      \"object\": { \"kind\": \"Object\", \"selectable\": true },\n"
            "      \"renderable\": { \"mesh\": \"builtin:cube\", \"visible\": false, \"albedo\": [0.369, 0.651, 0.961] }\n"
            "    },\n"
            "    {\n"
            "      \"id\": 2000,\n"
            "      \"name\": \"SampleCube\",\n"
            "      \"values\": { \"transform\": { \"position\": [0.75, 0.5, 0.75], \"rotation\": [0.0, 0.0, 0.0], \"scale\": [1.0, 1.0, 1.0] } },\n"
            "      \"overrides\": {},\n"
            "      \"object\": { \"kind\": \"Object\", \"selectable\": true },\n"
            "      \"renderable\": { \"mesh\": \"builtin:cube\", \"visible\": true, \"albedo\": [0.769, 0.808, 0.871] }\n"
            "    },\n"
            "    {\n"
            "      \"id\": 3000,\n"
            "      \"name\": \"MainLight\",\n"
            "      \"values\": { \"transform\": { \"position\": [-2.4, 3.2, -1.8], \"rotation\": [0.0, 0.0, 0.0], \"scale\": [1.0, 1.0, 1.0] } },\n"
            "      \"overrides\": {},\n"
            "      \"object\": { \"kind\": \"Light\", \"selectable\": true },\n"
            "      \"light\": {\n"
            "        \"position\": [-2.4, 3.2, -1.8],\n"
            "        \"color\": [1.0, 0.953, 0.839],\n"
            "        \"intensity\": 3.6,\n"
            "        \"range\": 22.0,\n"
            "        \"enabled\": true\n"
            "      }\n"
            "    }\n"
            "  ]\n"
            "}\n";
    }
}
