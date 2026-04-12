#pragma once

#include "prototypes/entity/scene/camera.hpp"
#include "prototypes/math/math.hpp"
#include "prototypes/presentation/frame.hpp"
#include "prototypes/presentation/view.hpp"
#include "systems/light_ray/light_ray_system.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

class EngineCore;

class SandboxApp
{
public:
    bool Run();

private:
    struct HexObservationCell
    {
        unsigned int id = 0U;
        unsigned int parentId = 0U;
        unsigned int depth = 0U;
        unsigned int clockwiseOrder = 0U;
        float ndcX = 0.0f;
        float ndcY = 0.0f;
        float ndcRadius = 0.35f;
        bool visible = true;
        bool sampled = false;
        bool subdivided = false;
        bool settled = false;
        bool active = false;
        Vector3 rayDirection = Vector3(0.0f, 0.0f, 1.0f);
        std::uint32_t sampleColor = 0xFF202020u;
        float aggregateRed = 0.0f;
        float aggregateGreen = 0.0f;
        float aggregateBlue = 0.0f;
        unsigned int aggregateSampleCount = 0U;
        unsigned int samplePassCount = 0U;
        unsigned int childPassCount = 0U;
        float priorityScore = 0.0f;
        float difficultyScore = 1.0f;
    };

    enum class HexObservationTaskKind : unsigned char
    {
        Sample,
        Refine
    };

    struct HexObservationTask
    {
        HexObservationTaskKind kind = HexObservationTaskKind::Sample;
        unsigned int cellId = 0U;
    };

    struct HexObservationSampleResult
    {
        unsigned int cellId = 0U;
        Vector3 rayDirection = Vector3(0.0f, 0.0f, 1.0f);
        std::uint32_t sampleColor = 0xFF202020u;
    };

    struct SandboxState
    {
        unsigned long long frameCount = 0;
        double totalTime = 0.0;
        double aspectRatio = 1.7777777777777777;
        CameraPrototype camera;
        Vector3 lightOrigin = Vector3(0.0f, 1.2f, -1.5f);
        Vector3 lightDirection = Vector3(0.0f, -0.08f, 1.0f);
        unsigned int rayCount = 32U;
        unsigned int randomRaySeed = 0U;
        float spreadAngleRadians = 1.1f;
        float maxDistance = 10.0f;
        double nextRandomRayRefreshTime = 0.0;
        bool showDebugRays = false;
        bool hexObservationMode = false;
        bool hexObservationCompleteLogged = false;
        double nextHexObservationStepTime = 0.0;
        double nextHexObservationRecursionTime = 0.0;
        unsigned int nextHexObservationId = 1U;
        unsigned int activeHexObservationId = 0U;
        unsigned int hexObservationAllowedRefineDepth = 0U;
        unsigned int hexObservationMaxSampledDepth = 0U;
        unsigned long long hexObservationGpuBatchCount = 0U;
        unsigned long long hexObservationCpuBatchCount = 0U;
        unsigned long long hexObservationGpuSampleCount = 0U;
        unsigned long long hexObservationCpuSampleCount = 0U;
        unsigned long long hexObservationGpuFallbackCount = 0U;
        bool lastHexObservationBatchUsedGpu = false;
    };

private:
    void UpdateFrame(EngineCore& core);
    void UpdateLogic(EngineCore& core);
    void UpdateCameraMovement(EngineCore& core);
    void UpdateLightControls(EngineCore& core);
    void UpdateRandomRayPattern();
    void ResetHexObservation();
    void UpdateHexObservation(EngineCore& core);
    void AdvanceHexObservationRecursionGate(EngineCore& core);
    unsigned int BuildHexObservationStepBudget(const EngineCore& core) const;
    std::size_t BuildHexObservationSampleBudget(const EngineCore& core) const;
    bool StepHexObservation(EngineCore& core, std::size_t sampleBudget);
    bool ProcessHexObservationSampleBatch(EngineCore& core, std::size_t sampleBudget);
    bool ProcessHexObservationRefineTask(EngineCore& core);
    HexObservationCell* FindHexObservationCell(unsigned int id);
    const HexObservationCell* FindHexObservationCell(unsigned int id) const;
    void ScheduleHexObservationGroup(const std::vector<unsigned int>& cellIds, bool sampleAtFront);
    void ApplyHexObservationSample(const HexObservationSampleResult& result, EngineCore& core);
    void UpdateHexObservationCellScore(HexObservationCell& cell);
    float ComputeHexObservationCellPriority(const HexObservationCell& cell) const;
    float ComputeHexObservationCellDifficulty(const HexObservationCell& cell) const;
    float ComputeHexObservationColorDifference(
        std::uint32_t firstColor,
        float secondRed,
        float secondGreen,
        float secondBlue) const;
    std::vector<unsigned int> SubdivideHexObservationCell(HexObservationCell& cell, EngineCore& core);
    void UpdateParentHexObservation(const HexObservationCell& child, EngineCore& core);
    Vector3 BuildHexObservationRay(float ndcX, float ndcY) const;
    std::uint32_t BuildHexObservationSampleColor(const Vector3& rayDirection, unsigned int depth) const;
    bool TrySampleHexObservationBox(
        const Vector3& rayOrigin,
        const Vector3& rayDirection,
        const Vector3& center,
        const Vector3& halfExtents,
        std::uint32_t color,
        float& inOutNearestDistance,
        std::uint32_t& outColor) const;
    bool TrySampleHexObservationPlane(
        const Vector3& rayOrigin,
        const Vector3& rayDirection,
        const Vector3& point,
        const Vector3& normal,
        std::uint32_t color,
        float& inOutNearestDistance,
        std::uint32_t& outColor) const;
    std::uint32_t ShadeHexObservationHit(
        std::uint32_t baseColor,
        const Vector3& hitNormal,
        const Vector3& rayDirection,
        float distance) const;
    std::uint32_t BuildHexObservationAggregateColor(const HexObservationCell& cell) const;
    void ResetLab();
    LightRayEmitterPrototype BuildLightEmitter() const;
    void UpdateLightRaySystem();
    void ConfigureLightRayTraceScene();
    FramePrototype BuildLightLabFrame();
    std::wstring BuildOverlay(const EngineCore& core) const;
    void AddCube(
        ViewPrototype& view,
        const Vector3& position,
        const Vector3& scale,
        const Vector3& rotation,
        std::uint32_t color) const;
    void AddRayMarker(
        ViewPrototype& view,
        const Vector3& origin,
        const Vector3& direction,
        float length,
        float thickness,
        std::uint32_t color) const;
    void AddHexObservationPatch(ViewPrototype& view, const HexObservationCell& cell) const;
    unsigned int CountLightRayHits() const;

private:
    SandboxState m_state;
    LightRaySystem m_lightRaySystem;
    std::vector<HexObservationCell> m_hexObservationCells;
    std::deque<HexObservationTask> m_hexObservationTasks;
};
