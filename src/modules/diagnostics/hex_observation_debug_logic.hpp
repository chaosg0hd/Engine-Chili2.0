#pragma once

#include "hex_observation_debug_types.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace diagnostics
{
struct HexObservationDebugConfig
{
    float startRadius = 0.36f;
    float childRadiusScale = 0.38f;
    float pixelLimit = 1.0f;
    float settleDifficulty = 0.055f;
};

class HexObservationDebugLogic
{
public:
    static HexObservationCell* FindCell(
        std::vector<HexObservationCell>& cells,
        unsigned int id);

    static const HexObservationCell* FindCell(
        const std::vector<HexObservationCell>& cells,
        unsigned int id);

    static std::vector<unsigned int> InitializeRootGroup(
        std::vector<HexObservationCell>& cells,
        unsigned int& nextId,
        const HexObservationDebugConfig& config = HexObservationDebugConfig{});

    static void ScheduleGroup(
        std::deque<HexObservationTask>& tasks,
        const std::vector<unsigned int>& cellIds,
        bool sampleAtFront);

    static bool HasReachedPixelLimit(
        const HexObservationCell& cell,
        int windowWidth,
        int windowHeight,
        const HexObservationDebugConfig& config = HexObservationDebugConfig{});

    static void UpdateCellScore(
        HexObservationCell& cell,
        const std::vector<HexObservationCell>& cells,
        const HexObservationDebugConfig& config = HexObservationDebugConfig{});

    static float ComputeCellPriority(const HexObservationCell& cell);

    static float ComputeCellDifficulty(
        const HexObservationCell& cell,
        const std::vector<HexObservationCell>& cells,
        const HexObservationDebugConfig& config = HexObservationDebugConfig{});

    static float ComputeColorDifference(
        std::uint32_t firstColor,
        float secondRed,
        float secondGreen,
        float secondBlue);

    static void ApplySample(
        std::vector<HexObservationCell>& cells,
        const HexObservationSampleResult& result,
        const HexObservationDebugConfig& config = HexObservationDebugConfig{});

    static std::vector<unsigned int> SubdivideCell(
        HexObservationCell& cell,
        std::vector<HexObservationCell>& cells,
        unsigned int& nextId,
        const HexObservationDebugConfig& config = HexObservationDebugConfig{});

    static void UpdateParentFromChild(
        const HexObservationCell& child,
        std::vector<HexObservationCell>& cells,
        const HexObservationDebugConfig& config = HexObservationDebugConfig{});

    static std::uint32_t BuildAggregateColor(const HexObservationCell& cell);
};
}
