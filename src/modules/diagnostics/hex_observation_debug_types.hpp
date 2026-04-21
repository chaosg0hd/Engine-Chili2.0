#pragma once

#include "../../prototypes/math/math.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace diagnostics
{
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
    Resample,
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
    bool blendExisting = false;
};

struct HexObservationBudgetContext
{
    unsigned int workerCount = 1U;
    unsigned int idleWorkerCount = 1U;
    unsigned int queuedJobCount = 0U;
    unsigned int activeJobCount = 0U;
    bool supportsComputeDispatch = false;
};

struct HexObservationBudgetConfig
{
    unsigned int maxStepsPerFrame = 128U;
    std::size_t maxSamplesPerBatch = 256U;
};
}
