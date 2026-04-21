#include "hex_observation_debug_budget.hpp"

#include <algorithm>

namespace diagnostics
{
unsigned int HexObservationDebugBudget::BuildStepBudget(
    const HexObservationBudgetContext& context,
    const HexObservationBudgetConfig& config)
{
    const unsigned int workerCount = std::max(1U, context.workerCount);
    const unsigned int idleWorkers = std::max(1U, context.idleWorkerCount);
    const unsigned int gpuMultiplier = context.supportsComputeDispatch ? 2U : 1U;
    const unsigned int pressure =
        (context.queuedJobCount > 0U || context.activeJobCount > 0U)
            ? 1U
            : idleWorkers;

    return std::max(
        1U,
        std::min(
            config.maxStepsPerFrame,
            pressure * gpuMultiplier * 4U + workerCount));
}

std::size_t HexObservationDebugBudget::BuildSampleBudget(
    const HexObservationBudgetContext& context,
    const HexObservationBudgetConfig& config)
{
    const unsigned int idleWorkers = std::max(1U, context.idleWorkerCount);
    const unsigned int gpuMultiplier = context.supportsComputeDispatch ? 2U : 1U;
    const std::size_t pressure =
        (context.queuedJobCount > 0U || context.activeJobCount > 0U)
            ? 1U
            : static_cast<std::size_t>(idleWorkers);

    return std::max<std::size_t>(
        1U,
        std::min<std::size_t>(
            config.maxSamplesPerBatch,
            pressure * static_cast<std::size_t>(gpuMultiplier) * 8U));
}
}
