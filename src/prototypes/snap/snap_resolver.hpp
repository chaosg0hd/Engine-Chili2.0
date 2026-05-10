#pragma once

#include "snap_debug.hpp"
#include "snap_settings.hpp"
#include "snap_types.hpp"

#include <vector>

struct SnapBlockTestQuery
{
    Vector3 probePoint = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 candidatePoint = Vector3(0.0f, 0.0f, 0.0f);
    const SnapCandidate* candidate = nullptr;
};

class ISnapBlockTest
{
public:
    virtual ~ISnapBlockTest() = default;
    virtual bool IsBlocked(const SnapBlockTestQuery& query) const = 0;
};

class SnapResolver
{
public:
    SnapResult Resolve(
        const SnapContext& context,
        const SnapSettings& settings,
        const std::vector<SnapCandidate>& candidates,
        const ISnapBlockTest* blockTest = nullptr,
        SnapDebugReport* debugReport = nullptr) const;
};

