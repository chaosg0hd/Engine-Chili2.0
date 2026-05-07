#pragma once

#include "runtime/scene_prototype.hpp"

#include "prototypes/entity/appearance/light.hpp"
#include "prototypes/entity/object/object.hpp"
#include "prototypes/entity/scene/camera.hpp"
#include "prototypes/presentation/frame.hpp"

namespace studio_runtime
{
    class SceneRuntime
    {
    public:
        void ResetToMinimalScene();
        void OrbitCamera(float yawDeltaRadians, float pitchDeltaRadians);
        FramePrototype BuildFrame() const;

    private:
        MaterialPrototype ResolveMaterial(const std::string& materialRef) const;

        ScenePrototype m_scene;
        CameraPrototype m_camera;
        LightPrototype m_light;
    };
}
