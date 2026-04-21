#include "hex_observation_debug_logic.hpp"

#include "../../prototypes/math/color_sample_math.hpp"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float HexObservationRingSpacing = 1.72f;
constexpr float HexObservationCenterBiasScale = 2.75f;
constexpr float HexObservationDifficultyCenterScale = 3.0f;
constexpr float HexObservationRgbSqrt3 = 1.7320508f;
}

namespace diagnostics
{
HexObservationCell* HexObservationDebugLogic::FindCell(
    std::vector<HexObservationCell>& cells,
    unsigned int id)
{
    for (HexObservationCell& cell : cells)
    {
        if (cell.id == id)
        {
            return &cell;
        }
    }

    return nullptr;
}

const HexObservationCell* HexObservationDebugLogic::FindCell(
    const std::vector<HexObservationCell>& cells,
    unsigned int id)
{
    for (const HexObservationCell& cell : cells)
    {
        if (cell.id == id)
        {
            return &cell;
        }
    }

    return nullptr;
}

std::vector<unsigned int> HexObservationDebugLogic::InitializeRootGroup(
    std::vector<HexObservationCell>& cells,
    unsigned int& nextId,
    const HexObservationDebugConfig& config)
{
    std::vector<unsigned int> rootGroupIds;
    rootGroupIds.reserve(7U);

    const auto addCell =
        [&](float ndcX, float ndcY, unsigned int clockwiseOrder)
        {
            HexObservationCell cell;
            cell.id = nextId++;
            cell.depth = 0U;
            cell.clockwiseOrder = clockwiseOrder;
            cell.ndcX = ndcX;
            cell.ndcY = ndcY;
            cell.ndcRadius = config.startRadius;
            cell.visible =
                std::fabs(ndcX) <= (1.0f + cell.ndcRadius) &&
                std::fabs(ndcY) <= (1.0f + cell.ndcRadius);
            UpdateCellScore(cell, cells, config);
            const unsigned int cellId = cell.id;
            cells.push_back(cell);
            return cellId;
        };

    rootGroupIds.push_back(addCell(0.0f, 0.0f, 0U));

    const float ringDistance = config.startRadius * HexObservationRingSpacing;
    for (unsigned int index = 0U; index < 6U; ++index)
    {
        const float angle = static_cast<float>(index) * 1.04719755f;
        rootGroupIds.push_back(addCell(
            std::sin(angle) * ringDistance,
            std::cos(angle) * ringDistance,
            index + 1U));
    }

    return rootGroupIds;
}

void HexObservationDebugLogic::ScheduleGroup(
    std::deque<HexObservationTask>& tasks,
    const std::vector<unsigned int>& cellIds,
    bool sampleAtFront)
{
    if (cellIds.empty())
    {
        return;
    }

    std::vector<HexObservationTask> sampleTasks;
    sampleTasks.reserve(cellIds.size());

    for (unsigned int cellId : cellIds)
    {
        sampleTasks.push_back(HexObservationTask{ HexObservationTaskKind::Sample, cellId });
    }

    if (sampleAtFront)
    {
        for (auto it = sampleTasks.rbegin(); it != sampleTasks.rend(); ++it)
        {
            tasks.push_front(*it);
        }
    }
    else
    {
        for (const HexObservationTask& task : sampleTasks)
        {
            tasks.push_back(task);
        }
    }

    for (unsigned int cellId : cellIds)
    {
        tasks.push_back(HexObservationTask{ HexObservationTaskKind::Refine, cellId });
    }
}

bool HexObservationDebugLogic::HasReachedPixelLimit(
    const HexObservationCell& cell,
    int windowWidth,
    int windowHeight,
    const HexObservationDebugConfig& config)
{
    if (windowWidth <= 0 || windowHeight <= 0)
    {
        return false;
    }

    const float smallerExtent = static_cast<float>(std::min(windowWidth, windowHeight));
    const float nextChildPixelRadius =
        cell.ndcRadius *
        config.childRadiusScale *
        0.5f *
        smallerExtent;
    return nextChildPixelRadius <= config.pixelLimit;
}

void HexObservationDebugLogic::UpdateCellScore(
    HexObservationCell& cell,
    const std::vector<HexObservationCell>& cells,
    const HexObservationDebugConfig& config)
{
    cell.difficultyScore = ComputeCellDifficulty(cell, cells, config);
    cell.priorityScore = ComputeCellPriority(cell);
}

float HexObservationDebugLogic::ComputeCellPriority(const HexObservationCell& cell)
{
    const float centerDistance = std::sqrt((cell.ndcX * cell.ndcX) + (cell.ndcY * cell.ndcY));
    const float centerBias = 1.0f / (1.0f + (centerDistance * HexObservationCenterBiasScale));
    const float depthCost = static_cast<float>(cell.depth) * 0.035f;
    const float parentBias = cell.parentId == 0U ? 0.2f : 0.0f;
    const float freshBias = cell.childPassCount == 0U ? 0.18f : 0.0f;
    const float visibilityBias = cell.visible ? 0.15f : -10.0f;
    const float settlePenalty = cell.settled ? 10.0f : 0.0f;
    return (centerBias * 2.4f) +
        (cell.difficultyScore * 2.1f) +
        parentBias +
        freshBias +
        visibilityBias -
        depthCost -
        settlePenalty;
}

float HexObservationDebugLogic::ComputeCellDifficulty(
    const HexObservationCell& cell,
    const std::vector<HexObservationCell>& cells,
    const HexObservationDebugConfig&)
{
    if (!cell.sampled)
    {
        return 1.0f;
    }

    float difficulty = 0.08f;
    const float centerDistance = std::sqrt((cell.ndcX * cell.ndcX) + (cell.ndcY * cell.ndcY));
    difficulty += 0.18f / (1.0f + (centerDistance * HexObservationDifficultyCenterScale));

    const HexObservationCell* parent = FindCell(cells, cell.parentId);
    if (parent && parent->aggregateSampleCount > 0U)
    {
        difficulty += ComputeColorDifference(
            cell.sampleColor,
            parent->aggregateRed,
            parent->aggregateGreen,
            parent->aggregateBlue) * 1.4f;
    }

    if (cell.childPassCount > 0U)
    {
        const float settling = std::min(0.45f, static_cast<float>(cell.childPassCount) * 0.055f);
        difficulty = std::max(0.0f, difficulty - settling);
    }

    return std::max(0.0f, std::min(1.0f, difficulty));
}

float HexObservationDebugLogic::ComputeColorDifference(
    std::uint32_t firstColor,
    float secondRed,
    float secondGreen,
    float secondBlue)
{
    float firstRed = 0.0f;
    float firstGreen = 0.0f;
    float firstBlue = 0.0f;
    UnpackRgbColor(firstColor, firstRed, firstGreen, firstBlue);
    const float redDifference = firstRed - secondRed;
    const float greenDifference = firstGreen - secondGreen;
    const float blueDifference = firstBlue - secondBlue;
    return std::sqrt(
        (redDifference * redDifference) +
        (greenDifference * greenDifference) +
        (blueDifference * blueDifference)) / HexObservationRgbSqrt3;
}

void HexObservationDebugLogic::ApplySample(
    std::vector<HexObservationCell>& cells,
    const HexObservationSampleResult& result,
    const HexObservationDebugConfig& config)
{
    HexObservationCell* cell = FindCell(cells, result.cellId);
    if (!cell || !cell->visible || (cell->sampled && !result.blendExisting))
    {
        return;
    }

    for (HexObservationCell& other : cells)
    {
        other.active = false;
    }

    cell->active = true;
    const bool wasSampled = cell->sampled;
    cell->sampled = true;
    cell->rayDirection = result.rayDirection;
    cell->sampleColor = result.sampleColor;
    float sampleRed = 0.0f;
    float sampleGreen = 0.0f;
    float sampleBlue = 0.0f;
    UnpackRgbColor(cell->sampleColor, sampleRed, sampleGreen, sampleBlue);
    if (wasSampled && result.blendExisting && cell->aggregateSampleCount > 0U)
    {
        const float currentCount = static_cast<float>(cell->aggregateSampleCount);
        cell->aggregateRed = ((cell->aggregateRed * currentCount) + sampleRed) / (currentCount + 1.0f);
        cell->aggregateGreen = ((cell->aggregateGreen * currentCount) + sampleGreen) / (currentCount + 1.0f);
        cell->aggregateBlue = ((cell->aggregateBlue * currentCount) + sampleBlue) / (currentCount + 1.0f);
        ++cell->aggregateSampleCount;
    }
    else
    {
        cell->aggregateRed = sampleRed;
        cell->aggregateGreen = sampleGreen;
        cell->aggregateBlue = sampleBlue;
        cell->aggregateSampleCount = std::max(1U, cell->aggregateSampleCount);
    }
    ++cell->samplePassCount;
    cell->settled = false;
    cell->difficultyScore = ComputeCellDifficulty(*cell, cells, config);
    cell->settled =
        cell->parentId != 0U &&
        cell->difficultyScore < config.settleDifficulty &&
        cell->depth > 1U;
    UpdateCellScore(*cell, cells, config);

    if (cell->parentId != 0U)
    {
        UpdateParentFromChild(*cell, cells, config);
    }
}

std::vector<unsigned int> HexObservationDebugLogic::SubdivideCell(
    HexObservationCell& cell,
    std::vector<HexObservationCell>& cells,
    unsigned int& nextId,
    const HexObservationDebugConfig& config)
{
    std::vector<unsigned int> childGroupIds;
    childGroupIds.reserve(7U);

    for (HexObservationCell& other : cells)
    {
        other.active = false;
    }

    const unsigned int parentId = cell.id;
    const unsigned int childDepth = cell.depth + 1U;
    const float parentNdcX = cell.ndcX;
    const float parentNdcY = cell.ndcY;
    const float childRadius = cell.ndcRadius * config.childRadiusScale;
    const float childDistance = childRadius * HexObservationRingSpacing;

    cell.subdivided = true;
    cell.active = true;

    const auto addChild =
        [&](float ndcX, float ndcY, unsigned int clockwiseOrder)
        {
            HexObservationCell child;
            child.id = nextId++;
            child.parentId = parentId;
            child.depth = childDepth;
            child.clockwiseOrder = clockwiseOrder;
            child.ndcX = ndcX;
            child.ndcY = ndcY;
            child.ndcRadius = childRadius;
            child.visible =
                std::fabs(ndcX) <= (1.0f + childRadius) &&
                std::fabs(ndcY) <= (1.0f + childRadius);
            UpdateCellScore(child, cells, config);
            const unsigned int childId = child.id;
            cells.push_back(child);
            return childId;
        };

    childGroupIds.push_back(addChild(parentNdcX, parentNdcY, 0U));
    for (unsigned int index = 0U; index < 6U; ++index)
    {
        const float angle = static_cast<float>(index) * 1.04719755f;
        childGroupIds.push_back(addChild(
            parentNdcX + (std::sin(angle) * childDistance),
            parentNdcY + (std::cos(angle) * childDistance),
            index + 1U));
    }

    return childGroupIds;
}

void HexObservationDebugLogic::UpdateParentFromChild(
    const HexObservationCell& child,
    std::vector<HexObservationCell>& cells,
    const HexObservationDebugConfig& config)
{
    HexObservationCell* parent = FindCell(cells, child.parentId);
    if (!parent)
    {
        return;
    }

    float childRed = 0.0f;
    float childGreen = 0.0f;
    float childBlue = 0.0f;
    UnpackRgbColor(child.sampleColor, childRed, childGreen, childBlue);

    const float currentCount = static_cast<float>(parent->aggregateSampleCount);
    parent->aggregateRed = ((parent->aggregateRed * currentCount) + childRed) / (currentCount + 1.0f);
    parent->aggregateGreen = ((parent->aggregateGreen * currentCount) + childGreen) / (currentCount + 1.0f);
    parent->aggregateBlue = ((parent->aggregateBlue * currentCount) + childBlue) / (currentCount + 1.0f);
    ++parent->aggregateSampleCount;
    ++parent->childPassCount;
    parent->difficultyScore = ComputeCellDifficulty(*parent, cells, config);
    parent->settled =
        parent->parentId != 0U &&
        parent->childPassCount >= 3U &&
        parent->difficultyScore < config.settleDifficulty;
    UpdateCellScore(*parent, cells, config);
}

std::uint32_t HexObservationDebugLogic::BuildAggregateColor(const HexObservationCell& cell)
{
    if (cell.aggregateSampleCount == 0U)
    {
        return cell.sampleColor;
    }

    return PackRgbColor(cell.aggregateRed, cell.aggregateGreen, cell.aggregateBlue);
}
}
