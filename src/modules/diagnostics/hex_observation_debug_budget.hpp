#pragma once

#include "hex_observation_debug_types.hpp"

namespace diagnostics
{
class HexObservationDebugBudget
{
public:
    static unsigned int BuildStepBudget(
        const HexObservationBudgetContext& context,
        const HexObservationBudgetConfig& config = HexObservationBudgetConfig{});

    static std::size_t BuildSampleBudget(
        const HexObservationBudgetContext& context,
        const HexObservationBudgetConfig& config = HexObservationBudgetConfig{});
};
}
