#pragma once

#include "../../prototypes/systems/light_ray.hpp"

#include <cstddef>
#include <vector>

class LightRaySystem
{
public:
    void ClearEmitters();
    void SetEmitters(const std::vector<LightRayEmitterPrototype>& emitters);
    void AddEmitter(const LightRayEmitterPrototype& emitter);
    void ClearTraceShapes();
    void AddPlane(const LightRayPlaneTraceShape& plane);
    void AddBox(const LightRayBoxTraceShape& box);

    void BeginFrame();
    void Update();

    const std::vector<LightRayEmitterPrototype>& GetEmitters() const;
    const std::vector<LightRayTraceShape>& GetTraceShapes() const;
    const std::vector<LightRayTraceResult>& GetTraceResults() const;

    std::size_t GetEmitterCount() const;
    std::size_t GetTraceShapeCount() const;
    std::size_t GetTraceResultCount() const;

private:
    static Vector3 BuildRayDirection(
        const LightRayEmitterPrototype& emitter,
        std::uint32_t rayIndex);
    static LightRayTraceResult BuildMissResult(
        const LightRayEmitterPrototype& emitter,
        std::uint32_t emitterIndex,
        std::uint32_t rayIndex);
    static LightRayTraceResult BuildBounceMissResult(
        const LightRayTraceResult& sourceHit);
    bool TryFindFirstHit(LightRayTraceResult& result) const;
    static bool TryTracePlane(
        const LightRayEmission& ray,
        const LightRayPlaneTraceShape& plane,
        LightRayTraceResult& result);
    static bool TryTraceBox(
        const LightRayEmission& ray,
        const LightRayBoxTraceShape& box,
        LightRayTraceResult& result);

private:
    std::vector<LightRayEmitterPrototype> m_emitters;
    std::vector<LightRayTraceShape> m_traceShapes;
    std::vector<LightRayTraceResult> m_traceResults;
};
