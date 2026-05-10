#include "runtime/studio_camera.hpp"

#include <cmath>
#include <algorithm>

namespace studio_runtime
{
    StudioCamera::StudioCamera()
    {
        m_camera.pose.position = Vector3(0.0f, 2.2f, -8.0f);
        m_camera.pose.target = m_orbitTarget;
        m_camera.pose.useTarget = true;
        m_camera.purpose = CameraPurposePrototype::Preview;
        m_camera.SetPerspective(60.0f, 0.1f, 500.0f);
    }

    const CameraPrototype& StudioCamera::GetCamera() const
    {
        return m_camera;
    }

    CameraPrototype& StudioCamera::GetCamera()
    {
        return m_camera;
    }

    void StudioCamera::SetOrbitTarget(const Vector3& target)
    {
        m_orbitTarget = target;
        m_camera.LookAt(target);
    }

    void StudioCamera::FocusOn(const Vector3& target, float distance)
    {
        const Vector3 forward = m_camera.GetForward();
        const float resolvedDistance = std::max(1.0f, distance);
        m_orbitTarget = target;
        m_camera.pose.position = target - (forward * resolvedDistance);
        m_camera.LookAt(target);
    }

    void StudioCamera::Orbit(float yawDeltaRadians, float pitchDeltaRadians)
    {
        m_camera.OrbitAround(m_orbitTarget, yawDeltaRadians, pitchDeltaRadians);
    }

    void StudioCamera::Look(float yawDeltaRadians, float pitchDeltaRadians)
    {
        const Vector3 forward = m_camera.GetForward();
        const float currentYaw = std::atan2(forward.x, forward.z);
        float currentPitch = std::asin(std::max(-1.0f, std::min(1.0f, forward.y)));
        currentPitch += pitchDeltaRadians;

        const float pitchLimit = 1.55334306f;
        if (currentPitch > pitchLimit)
        {
            currentPitch = pitchLimit;
        }
        else if (currentPitch < -pitchLimit)
        {
            currentPitch = -pitchLimit;
        }

        const float nextYaw = currentYaw + yawDeltaRadians;
        const float cosPitch = std::cos(currentPitch);
        const Vector3 nextForward(
            std::sin(nextYaw) * cosPitch,
            std::sin(currentPitch),
            std::cos(nextYaw) * cosPitch);

        const float targetDistance = std::max(1.0f, Length(m_camera.GetTargetPosition() - m_camera.GetWorldPosition()));
        m_camera.LookAt(m_camera.GetWorldPosition() + (Normalize(nextForward) * targetDistance));
        m_orbitTarget = m_camera.GetTargetPosition();
    }

    void StudioCamera::Pan(float rightDistance, float upDistance)
    {
        const Vector3 delta = (m_camera.GetRight() * rightDistance) + (m_camera.GetUpDirection() * upDistance);
        m_orbitTarget = m_orbitTarget + delta;
        m_camera.Move(delta);
    }

    void StudioCamera::Dolly(float distance)
    {
        m_camera.MoveForward(distance);
    }

    void StudioCamera::DollyToward(const Vector3& direction, float distance)
    {
        const Vector3 delta = direction * distance;
        m_camera.Move(delta);
        m_orbitTarget = m_orbitTarget + delta;
    }

    bool StudioCamera::TryScreenPointToRay(
        int screenX,
        int screenY,
        const ViewportRect& viewportRect,
        Ray& outRay) const
    {
        if (!viewportRect.Contains(screenX, screenY))
        {
            return false;
        }

        const int width = std::max(1, viewportRect.width);
        const int height = std::max(1, viewportRect.height);
        const float localX = static_cast<float>(screenX - viewportRect.x) / static_cast<float>(width);
        const float localY = static_cast<float>(screenY - viewportRect.y) / static_cast<float>(height);
        const float ndcX = (localX * 2.0f) - 1.0f;
        const float ndcY = 1.0f - (localY * 2.0f);
        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        const float tanHalfFov = std::tan(m_camera.projection.fieldOfViewDegrees * 0.01745329251994329577f * 0.5f);

        const Vector3 direction = Normalize(
            m_camera.GetForward() +
            (m_camera.GetRight() * (ndcX * tanHalfFov * aspect)) +
            (m_camera.GetUpDirection() * (ndcY * tanHalfFov)));

        outRay.origin = m_camera.GetWorldPosition();
        outRay.direction = Length(direction) > 0.000001f ? direction : m_camera.GetForward();
        return true;
    }
}
