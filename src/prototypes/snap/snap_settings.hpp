#pragma once

#include "snap_types.hpp"

#include <vector>

struct SnapTypePriority
{
    SnapCandidateType type = SnapCandidateType::Point;
    int score = 0;
};

struct SnapToolPreference
{
    SnapToolKind tool = SnapToolKind::Generic;
    SnapCandidateType type = SnapCandidateType::Point;
    int bonusScore = 0;
};

struct SnapSettings
{
    // Stage 1: distance threshold gate
    float maxSnapDistance = 1.0f;

    // Stage 2: occlusion gate/penalty
    bool rejectBlockedCandidates = false;
    float blockedDistancePenalty = 1.5f;

    // Stage 3: confidence gate
    float minimumConfidence = 0.05f;

    // Stage 4: weighted score blend (higher is better)
    float distanceWeight = 1.0f;
    float screenDistanceWeight = 0.75f;
    float typePriorityWeight = 0.01f;
    float toolPreferenceWeight = 0.01f;
    float blockedPenaltyWeight = 0.35f;
    float screenDistanceSoftMaxPx = 24.0f;
    float maxScreenSnapErrorPx = 48.0f;

    // Stage 5: deterministic tie-break epsilon
    float tieBreakEpsilon = 0.0001f;

    std::vector<SnapTypePriority> typePriorities;
    std::vector<SnapToolPreference> toolPreferences;
    std::vector<SnapCandidateType> enabledTypes;
};

SnapSettings CreateDefaultSnapSettings();
SnapSettings CreateVertexPointOriginSnapSettings();
