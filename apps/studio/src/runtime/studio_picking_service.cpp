#include "runtime/studio_picking_service.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <sstream>

namespace studio_runtime
{
    StudioPickingService::StudioPickingService(const RuntimeWorld& world, const StudioCamera& camera)
        : m_world(world)
        , m_camera(camera)
    {
    }

    void StudioPickingService::SetConfig(const StudioPickingConfig& config)
    {
        m_config = config;
    }

    const StudioPickingConfig& StudioPickingService::GetConfig() const
    {
        return m_config;
    }

    StudioPickResult StudioPickingService::Pick(const StudioPickRequest& request) const
    {
        StudioPickResult result;

        if (!request.viewport.Contains(request.mouseX, request.mouseY))
        {
            if (m_config.enableDebugLogging)
            {
                result.debugLog = "Pick rejected: mouse outside viewport";
            }
            return result;
        }

        // Ray generated with the same viewport rect used by the renderer — ensures
        // the NDC mapping matches the projection matrix used when drawing the frame.
        Ray ray;
        if (!m_camera.TryScreenPointToRay(request.mouseX, request.mouseY, request.viewport, ray))
        {
            if (m_config.enableDebugLogging)
            {
                result.debugLog = "Pick rejected: ray generation failed";
            }
            return result;
        }

        std::ostringstream log;
        if (m_config.enableDebugLogging)
        {
            log << "Pick mouse(" << request.mouseX << "," << request.mouseY << ")";
            log << " vp(" << request.viewport.x << "," << request.viewport.y
                << " " << request.viewport.width << "x" << request.viewport.height << ")";
            log << " ray.o(" << ray.origin.x << "," << ray.origin.y << "," << ray.origin.z << ")";
            log << " ray.d(" << ray.direction.x << "," << ray.direction.y << "," << ray.direction.z << ")";
        }

        for (const EntityId id : m_world.GetEntityList())
        {
            EntityInfo info;
            if (!m_world.GetEntityInfo(id, info) || !info.hasRenderable || !info.renderable.visible)
            {
                continue;
            }
            if (info.hasObject && !info.object.selectable)
            {
                continue;
            }

            // Minimum distance from entity center to the ray line (not sphere intersection).
            // Using continuous distance gives valid scores even outside the picking volume,
            // which the screen-pixel fallback path relies on.
            const Vector3 toEntity = info.transform.translation - ray.origin;
            const float projectedT = std::max(0.0f, Dot(toEntity, ray.direction));
            const Vector3 closestOnRay = ray.origin + ray.direction * projectedT;
            const float worldDistanceToRay = Length(info.transform.translation - closestOnRay);

            const float effectiveRadius = ComputeEffectivePickRadius(info);
            const float screenDistancePx = ComputeScreenDistancePx(
                info.transform.translation, request.mouseX, request.mouseY, request.viewport);

            // Two acceptance conditions — either is sufficient.
            // Volume hit handles direct clicks; screen tolerance handles near-misses.
            const bool hitsVolume = worldDistanceToRay <= effectiveRadius;
            const bool closeOnScreen = screenDistancePx <= m_config.screenPixelTolerance;
            if (!hitsVolume && !closeOnScreen)
            {
                continue;
            }

            // Orthogonal depth along camera forward axis — closer objects score better
            // when depth weight is non-zero. Ties in screen distance resolve by depth.
            const float depth = Dot(
                info.transform.translation - ray.origin,
                m_camera.GetCamera().GetForward());

            StudioPickCandidate candidate;
            candidate.entity = id;
            candidate.worldDistanceToRay = worldDistanceToRay;
            candidate.screenDistancePx = screenDistancePx;
            candidate.depth = depth;
            candidate.score = ComputeScore(candidate);
            result.candidates.push_back(candidate);
        }

        if (m_config.enableDebugLogging)
        {
            log << " candidates:" << result.candidates.size();
        }

        if (result.candidates.empty())
        {
            if (m_config.enableDebugLogging)
            {
                log << " MISS";
                result.debugLog = log.str();
            }
            return result;
        }

        const auto best = std::min_element(
            result.candidates.begin(),
            result.candidates.end(),
            [](const StudioPickCandidate& a, const StudioPickCandidate& b) {
                return a.score < b.score;
            });

        result.hit = true;
        result.entity = best->entity;
        result.bestCandidate = *best;

        if (m_config.enableDebugLogging)
        {
            log << " HIT entity:" << result.entity;
            log << " score:" << best->score;
            log << " worldDist:" << best->worldDistanceToRay;
            log << " screenDist:" << best->screenDistancePx;
            log << " depth:" << best->depth;
            result.debugLog = log.str();
        }

        return result;
    }

    float StudioPickingService::ComputeEffectivePickRadius(const EntityInfo& info) const
    {
        const Vector3& scale = info.transform.scale;
        const float maxScale = std::max({scale.x, scale.y, scale.z});

        float radius;
        if (info.hasRenderable)
        {
            switch (info.renderable.mesh)
            {
            case BuiltInMeshKind::Cube:
                // Circumscribed sphere of a unit cube: sqrt(3)/2 * scale ≈ 0.866 * scale
                radius = 0.866f * maxScale;
                break;
            case BuiltInMeshKind::Octahedron:
                // Circumscribed sphere of a unit octahedron: sqrt(2)/2 * scale ≈ 0.707 * scale
                radius = 0.707f * maxScale;
                break;
            default:
                // Conservative inscribed-sphere fallback for Quad/Triangle/Diamond
                radius = 0.5f * maxScale;
                break;
            }
        }
        else
        {
            radius = 0.5f * maxScale;
        }

        return std::max(radius * m_config.baseSphereRadiusMultiplier, m_config.minimumPickRadius);
    }

    float StudioPickingService::ComputeScreenDistancePx(
        const Vector3& worldPoint,
        int mouseX,
        int mouseY,
        const ViewportRect& viewport) const
    {
        const CameraPrototype& cam = m_camera.GetCamera();
        const Vector3 toPoint = worldPoint - cam.GetWorldPosition();
        const float projForward = Dot(toPoint, cam.GetForward());

        if (projForward <= 0.0001f)
        {
            return std::numeric_limits<float>::max();
        }

        const int width = std::max(1, viewport.width);
        const int height = std::max(1, viewport.height);
        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        const float tanHalfFov = std::tan(
            cam.projection.fieldOfViewDegrees * 0.01745329251994329577f * 0.5f);

        // Reverse projection — mirrors the math in TryScreenPointToRay so the two
        // coordinate spaces are guaranteed to be consistent.
        const float ndcX = Dot(toPoint, cam.GetRight()) / (projForward * tanHalfFov * aspect);
        const float ndcY = Dot(toPoint, cam.GetUpDirection()) / (projForward * tanHalfFov);

        const float screenX = (ndcX + 1.0f) * 0.5f * static_cast<float>(width)
                            + static_cast<float>(viewport.x);
        const float screenY = (1.0f - ndcY) * 0.5f * static_cast<float>(height)
                            + static_cast<float>(viewport.y);

        const float dx = screenX - static_cast<float>(mouseX);
        const float dy = screenY - static_cast<float>(mouseY);
        return std::sqrt(dx * dx + dy * dy);
    }

    float StudioPickingService::ComputeScore(const StudioPickCandidate& candidate) const
    {
        // Lower score = better match. Screen distance is weighted highest so that
        // visually close objects win over objects that are technically nearer in world
        // space but far from the cursor on screen.
        return candidate.worldDistanceToRay * m_config.distanceWeight
             + candidate.screenDistancePx   * m_config.screenDistanceWeight
             + candidate.depth              * m_config.depthWeight;
    }
}
