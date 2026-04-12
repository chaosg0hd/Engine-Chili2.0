#include "light_ray_system.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float kMinimumDirectionLength = 0.000001f;
    constexpr float kGoldenAngleRadians = 2.39996323f;
    constexpr float kBounceOriginOffset = 0.002f;

    Vector3 GetFallbackDirection()
    {
        return Vector3(0.0f, -1.0f, 0.0f);
    }

    std::uint32_t HashRayValue(std::uint32_t seed, std::uint32_t rayIndex, std::uint32_t salt)
    {
        std::uint32_t value = seed ^ (rayIndex * 0x9E3779B9u) ^ salt;
        value ^= value >> 16U;
        value *= 0x7FEB352Du;
        value ^= value >> 15U;
        value *= 0x846CA68Bu;
        value ^= value >> 16U;
        return value;
    }

    float ToUnitFloat(std::uint32_t value)
    {
        return static_cast<float>(value & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

    Vector3 NormalizeOrFallback(const Vector3& value, const Vector3& fallback)
    {
        const Vector3 normalized = Normalize(value);
        return Length(normalized) > kMinimumDirectionLength ? normalized : fallback;
    }

    void BuildBasis(const Vector3& forward, Vector3& right, Vector3& up)
    {
        const Vector3 worldUp = (std::fabs(forward.y) < 0.98f)
            ? Vector3(0.0f, 1.0f, 0.0f)
            : Vector3(1.0f, 0.0f, 0.0f);

        right = NormalizeOrFallback(Cross(worldUp, forward), Vector3(1.0f, 0.0f, 0.0f));
        up = NormalizeOrFallback(Cross(forward, right), Vector3(0.0f, 1.0f, 0.0f));
    }
}

void LightRaySystem::ClearEmitters()
{
    m_emitters.clear();
}

void LightRaySystem::SetEmitters(const std::vector<LightRayEmitterPrototype>& emitters)
{
    m_emitters = emitters;
}

void LightRaySystem::AddEmitter(const LightRayEmitterPrototype& emitter)
{
    m_emitters.push_back(emitter);
}

void LightRaySystem::ClearTraceShapes()
{
    m_traceShapes.clear();
}

void LightRaySystem::AddPlane(const LightRayPlaneTraceShape& plane)
{
    LightRayTraceShape shape;
    shape.kind = LightRayTraceShapeKind::Plane;
    shape.plane = plane;
    m_traceShapes.push_back(shape);
}

void LightRaySystem::AddBox(const LightRayBoxTraceShape& box)
{
    LightRayTraceShape shape;
    shape.kind = LightRayTraceShapeKind::Box;
    shape.box = box;
    m_traceShapes.push_back(shape);
}

void LightRaySystem::BeginFrame()
{
    m_traceResults.clear();
}

void LightRaySystem::Update()
{
    BeginFrame();

    std::size_t resultCapacity = 0U;
    for (const LightRayEmitterPrototype& emitter : m_emitters)
    {
        if (emitter.enabled && emitter.IsValid())
        {
            resultCapacity += emitter.rayCount;
        }
    }

    m_traceResults.reserve(resultCapacity);
    for (std::uint32_t emitterIndex = 0U; emitterIndex < static_cast<std::uint32_t>(m_emitters.size()); ++emitterIndex)
    {
        const LightRayEmitterPrototype& emitter = m_emitters[emitterIndex];
        if (!emitter.enabled || !emitter.IsValid())
        {
            continue;
        }

        for (std::uint32_t rayIndex = 0U; rayIndex < emitter.rayCount; ++rayIndex)
        {
            LightRayTraceResult result = BuildMissResult(emitter, emitterIndex, rayIndex);
            TryFindFirstHit(result);
            m_traceResults.push_back(result);

            if (emitter.maxBounceCount > 0U && result.DidHit() && result.hitMaterial.IsReflective())
            {
                LightRayTraceResult bounceResult = BuildBounceMissResult(result);
                if (bounceResult.emittedRay.maxDistance > kMinimumDirectionLength)
                {
                    TryFindFirstHit(bounceResult);
                    m_traceResults.push_back(bounceResult);
                }
            }
        }
    }
}

const std::vector<LightRayEmitterPrototype>& LightRaySystem::GetEmitters() const
{
    return m_emitters;
}

const std::vector<LightRayTraceShape>& LightRaySystem::GetTraceShapes() const
{
    return m_traceShapes;
}

const std::vector<LightRayTraceResult>& LightRaySystem::GetTraceResults() const
{
    return m_traceResults;
}

std::size_t LightRaySystem::GetEmitterCount() const
{
    return m_emitters.size();
}

std::size_t LightRaySystem::GetTraceShapeCount() const
{
    return m_traceShapes.size();
}

std::size_t LightRaySystem::GetTraceResultCount() const
{
    return m_traceResults.size();
}

Vector3 LightRaySystem::BuildRayDirection(
    const LightRayEmitterPrototype& emitter,
    std::uint32_t rayIndex)
{
    const Vector3 forward = NormalizeOrFallback(emitter.direction, GetFallbackDirection());
    if (emitter.rayCount <= 1U || emitter.spreadAngleRadians <= 0.0f)
    {
        return forward;
    }

    Vector3 right;
    Vector3 up;
    BuildBasis(forward, right, up);

    const float normalizedIndex = emitter.randomSeed == 0U
        ? ((static_cast<float>(rayIndex) + 0.5f) / static_cast<float>(emitter.rayCount))
        : ToUnitFloat(HashRayValue(emitter.randomSeed, rayIndex, 0xA511E9B3u));
    const float coneRadius = std::sqrt(normalizedIndex);
    const float halfSpreadAngle = emitter.spreadAngleRadians * 0.5f;
    const float rayAngle = halfSpreadAngle * coneRadius;
    const float spinAngle = emitter.randomSeed == 0U
        ? (static_cast<float>(rayIndex) * kGoldenAngleRadians)
        : (ToUnitFloat(HashRayValue(emitter.randomSeed, rayIndex, 0x63D83595u)) * 6.2831853f);
    const Vector3 radialDirection =
        (right * std::cos(spinAngle)) +
        (up * std::sin(spinAngle));

    return NormalizeOrFallback(
        (forward * std::cos(rayAngle)) + (radialDirection * std::sin(rayAngle)),
        forward);
}

LightRayTraceResult LightRaySystem::BuildMissResult(
    const LightRayEmitterPrototype& emitter,
    std::uint32_t emitterIndex,
    std::uint32_t rayIndex)
{
    LightRayTraceResult result;
    result.emittedRay.emitterIndex = emitterIndex;
    result.emittedRay.rayIndex = rayIndex;
    result.emittedRay.bounceDepth = 0U;
    result.emittedRay.origin = emitter.origin;
    result.emittedRay.direction = BuildRayDirection(emitter, rayIndex);
    result.emittedRay.color = emitter.color;
    result.emittedRay.intensity = emitter.intensity;
    result.emittedRay.maxDistance = emitter.maxDistance;
    result.hitState = LightRayHitState::Miss;
    result.hitPosition = emitter.origin + (result.emittedRay.direction * emitter.maxDistance);
    result.hitDistance = 0.0f;
    return result;
}

LightRayTraceResult LightRaySystem::BuildBounceMissResult(
    const LightRayTraceResult& sourceHit)
{
    const Vector3 incomingDirection = NormalizeOrFallback(sourceHit.emittedRay.direction, GetFallbackDirection());
    const Vector3 hitNormal = NormalizeOrFallback(sourceHit.hitNormal, Vector3(0.0f, 1.0f, 0.0f));
    const Vector3 reflectedDirection = NormalizeOrFallback(
        sourceHit.hitMaterial.ComputeReflectedDirection(incomingDirection, hitNormal),
        incomingDirection);
    const float remainingDistance = sourceHit.emittedRay.maxDistance - sourceHit.hitDistance;
    const float reflectedIntensity =
        sourceHit.hitMaterial.ApplyAbsorptionToIntensity(
            sourceHit.hitMaterial.ApplyReflectivityToIntensity(sourceHit.emittedRay.intensity));

    LightRayTraceResult result;
    result.emittedRay = sourceHit.emittedRay;
    result.emittedRay.bounceDepth = sourceHit.emittedRay.bounceDepth + 1U;
    result.emittedRay.origin = sourceHit.hitPosition + (hitNormal * kBounceOriginOffset);
    result.emittedRay.direction = reflectedDirection;
    result.emittedRay.intensity = reflectedIntensity;
    result.emittedRay.maxDistance = remainingDistance;
    result.hitState = LightRayHitState::Miss;
    result.hitPosition = result.emittedRay.origin + (reflectedDirection * remainingDistance);
    result.hitNormal = Vector3(0.0f, 1.0f, 0.0f);
    result.hitDistance = 0.0f;
    return result;
}

bool LightRaySystem::TryFindFirstHit(LightRayTraceResult& result) const
{
    bool didHit = false;
    float nearestDistance = result.emittedRay.maxDistance;
    LightRayTraceResult nearestResult = result;

    for (const LightRayTraceShape& shape : m_traceShapes)
    {
        LightRayTraceResult candidate = result;
        bool shapeHit = false;
        switch (shape.kind)
        {
        case LightRayTraceShapeKind::Plane:
            shapeHit = TryTracePlane(result.emittedRay, shape.plane, candidate);
            break;
        case LightRayTraceShapeKind::Box:
            shapeHit = TryTraceBox(result.emittedRay, shape.box, candidate);
            break;
        default:
            break;
        }

        if (shapeHit && candidate.hitDistance <= nearestDistance)
        {
            didHit = true;
            nearestDistance = candidate.hitDistance;
            nearestResult = candidate;
        }
    }

    if (didHit)
    {
        result = nearestResult;
    }

    return didHit;
}

bool LightRaySystem::TryTracePlane(
    const LightRayEmission& ray,
    const LightRayPlaneTraceShape& plane,
    LightRayTraceResult& result)
{
    const Vector3 normal = NormalizeOrFallback(plane.normal, Vector3(0.0f, 1.0f, 0.0f));
    const float denominator = Dot(ray.direction, normal);
    if (std::fabs(denominator) <= kMinimumDirectionLength)
    {
        return false;
    }

    const float distance = Dot(plane.point - ray.origin, normal) / denominator;
    if (distance < 0.0f || distance > ray.maxDistance)
    {
        return false;
    }

    result.emittedRay = ray;
    result.hitState = LightRayHitState::Hit;
    result.hitPosition = ray.origin + (ray.direction * distance);
    result.hitNormal = denominator > 0.0f ? (normal * -1.0f) : normal;
    result.hitDistance = distance;
    result.hitMaterial = plane.material;
    return true;
}

bool LightRaySystem::TryTraceBox(
    const LightRayEmission& ray,
    const LightRayBoxTraceShape& box,
    LightRayTraceResult& result)
{
    const Vector3 minCorner = box.center - box.halfExtents;
    const Vector3 maxCorner = box.center + box.halfExtents;
    float tMin = 0.0f;
    float tMax = ray.maxDistance;
    Vector3 hitNormal = Vector3(0.0f, 1.0f, 0.0f);

    auto traceAxis =
        [&](float origin, float direction, float minValue, float maxValue, const Vector3& axisNormal) -> bool
        {
            if (std::fabs(direction) <= kMinimumDirectionLength)
            {
                return origin >= minValue && origin <= maxValue;
            }

            float nearDistance = (minValue - origin) / direction;
            float farDistance = (maxValue - origin) / direction;
            Vector3 nearNormal = axisNormal * -1.0f;

            if (nearDistance > farDistance)
            {
                std::swap(nearDistance, farDistance);
                nearNormal = axisNormal;
            }

            if (nearDistance > tMin)
            {
                tMin = nearDistance;
                hitNormal = nearNormal;
            }

            tMax = std::min(tMax, farDistance);
            return tMin <= tMax;
        };

    if (!traceAxis(ray.origin.x, ray.direction.x, minCorner.x, maxCorner.x, Vector3(1.0f, 0.0f, 0.0f)) ||
        !traceAxis(ray.origin.y, ray.direction.y, minCorner.y, maxCorner.y, Vector3(0.0f, 1.0f, 0.0f)) ||
        !traceAxis(ray.origin.z, ray.direction.z, minCorner.z, maxCorner.z, Vector3(0.0f, 0.0f, 1.0f)))
    {
        return false;
    }

    const float hitDistance = tMin >= 0.0f ? tMin : tMax;
    if (hitDistance < 0.0f || hitDistance > ray.maxDistance)
    {
        return false;
    }

    result.emittedRay = ray;
    result.hitState = LightRayHitState::Hit;
    result.hitPosition = ray.origin + (ray.direction * hitDistance);
    result.hitNormal = hitNormal;
    result.hitDistance = hitDistance;
    result.hitMaterial = box.material;
    return true;
}
