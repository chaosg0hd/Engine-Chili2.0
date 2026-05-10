#pragma once

#include "prototypes/entity/scene/camera.hpp"
#include "modules/render/render_types.hpp"

namespace studio_runtime
{
    struct Ray
    {
        Vector3 origin;
        Vector3 direction = Vector3(0.0f, 0.0f, 1.0f);
    };

    class StudioCamera
    {
    public:
        StudioCamera();

        const CameraPrototype& GetCamera() const;
        CameraPrototype& GetCamera();
        void SetOrbitTarget(const Vector3& target);
        void FocusOn(const Vector3& target, float distance = 6.0f);
        void Orbit(float yawDeltaRadians, float pitchDeltaRadians);
        void Look(float yawDeltaRadians, float pitchDeltaRadians);
        void Pan(float rightDistance, float upDistance);
        void Dolly(float distance);
        void DollyToward(const Vector3& direction, float distance);
        bool TryScreenPointToRay(int screenX, int screenY, const ViewportRect& viewportRect, Ray& outRay) const;

    private:
        CameraPrototype m_camera;
        Vector3 m_orbitTarget = Vector3(0.0f, 0.0f, 0.0f);
    };
}
