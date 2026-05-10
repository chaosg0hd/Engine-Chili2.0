#pragma once

#include "snap_types.hpp"

#include <string>
#include <vector>

struct SnapDecisionEntry
{
    std::string label;
    SnapCandidateType type = SnapCandidateType::Point;
    Vector3 resolvedPosition = Vector3(0.0f, 0.0f, 0.0f);
    float distance = 0.0f;
    bool blocked = false;
    bool accepted = false;
    std::string reason;
};

struct SnapDebugReport
{
    std::vector<SnapDecisionEntry> entries;
    std::string summary;
};

