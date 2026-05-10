#include "snap_resolver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace
{
    constexpr float kEpsilon = 0.000001f;

    float Distance(const Vector3& a, const Vector3& b)
    {
        return Length(a - b);
    }

    float DistancePointToRay(const Vector3& point, const SnapRay& ray)
    {
        const Vector3 direction = Normalize(ray.direction);
        if (Length(direction) <= kEpsilon)
        {
            return Distance(point, ray.origin);
        }

        const Vector3 toPoint = point - ray.origin;
        const float t = std::max(0.0f, Dot(toPoint, direction));
        const Vector3 closest = ray.origin + (direction * t);
        return Distance(point, closest);
    }

    float Clamp01(float value)
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    int GetTypePriorityScore(const SnapSettings& settings, SnapCandidateType type)
    {
        for (const SnapTypePriority& item : settings.typePriorities)
        {
            if (item.type == type)
            {
                return item.score;
            }
        }

        return 0;
    }

    bool IsTypeEnabled(const SnapSettings& settings, SnapCandidateType type)
    {
        if (settings.enabledTypes.empty())
        {
            return true;
        }

        for (const SnapCandidateType enabled : settings.enabledTypes)
        {
            if (enabled == type)
            {
                return true;
            }
        }

        return false;
    }

    int GetToolPreferenceBonus(
        const SnapSettings& settings,
        SnapToolKind tool,
        SnapCandidateType type)
    {
        for (const SnapToolPreference& item : settings.toolPreferences)
        {
            if (item.tool == tool && item.type == type)
            {
                return item.bonusScore;
            }
        }

        return 0;
    }

    bool ResolveProbePoint(const SnapContext& context, Vector3& outProbePoint)
    {
        if (context.probe.hasPoint)
        {
            outProbePoint = context.probe.point;
            return true;
        }

        if (context.probe.hasRay)
        {
            outProbePoint = context.probe.ray.origin;
            return true;
        }

        return false;
    }

    Vector3 ResolveCandidatePosition(
        const SnapCandidate& candidate,
        const Vector3& probePoint)
    {
        if (candidate.hasEdge)
        {
            const Vector3 edge = candidate.edgeEnd - candidate.edgeStart;
            const float lengthSq = Dot(edge, edge);
            if (lengthSq > kEpsilon)
            {
                const float t = Clamp01(Dot(probePoint - candidate.edgeStart, edge) / lengthSq);
                return candidate.edgeStart + (edge * t);
            }
        }

        if (candidate.hasPlane)
        {
            const Vector3 normal = Normalize(candidate.planeNormal);
            const float normalLengthSq = Dot(normal, normal);
            if (normalLengthSq > kEpsilon)
            {
                const float signedDistance = Dot(probePoint - candidate.planePoint, normal);
                return probePoint - (normal * signedDistance);
            }
        }

        return candidate.worldPosition;
    }

    std::string CandidateName(SnapCandidateType type)
    {
        switch (type)
        {
        case SnapCandidateType::HelperPoint: return "HelperPoint";
        case SnapCandidateType::Point: return "Point";
        case SnapCandidateType::Edge: return "Edge";
        case SnapCandidateType::FacePlane: return "FacePlane";
        case SnapCandidateType::Grid: return "Grid";
        case SnapCandidateType::ObjectOrigin: return "ObjectOrigin";
        case SnapCandidateType::BoundingBoxCorner: return "BoundingBoxCorner";
        case SnapCandidateType::BoundingBoxCenter: return "BoundingBoxCenter";
        default: return "Unknown";
        }
    }
}

SnapSettings CreateDefaultSnapSettings()
{
    SnapSettings settings;
    settings.maxSnapDistance = 1.0f;
    settings.rejectBlockedCandidates = false;
    settings.blockedDistancePenalty = 1.5f;
    settings.minimumConfidence = 0.05f;
    settings.typePriorities =
    {
        { SnapCandidateType::HelperPoint, 800 },
        { SnapCandidateType::Point, 700 },
        { SnapCandidateType::Edge, 600 },
        { SnapCandidateType::ObjectOrigin, 500 },
        { SnapCandidateType::BoundingBoxCorner, 400 },
        { SnapCandidateType::BoundingBoxCenter, 300 },
        { SnapCandidateType::FacePlane, 200 },
        { SnapCandidateType::Grid, 100 }
    };
    settings.toolPreferences =
    {
        { SnapToolKind::Translate, SnapCandidateType::Point, 35 },
        { SnapToolKind::Translate, SnapCandidateType::Edge, 25 },
        { SnapToolKind::Rotate, SnapCandidateType::ObjectOrigin, 35 },
        { SnapToolKind::Scale, SnapCandidateType::BoundingBoxCorner, 35 }
    };
    settings.enabledTypes.clear();
    return settings;
}

SnapSettings CreateVertexPointOriginSnapSettings()
{
    SnapSettings settings = CreateDefaultSnapSettings();
    settings.enabledTypes =
    {
        SnapCandidateType::Point,
        SnapCandidateType::ObjectOrigin
    };
    return settings;
}

SnapResult SnapResolver::Resolve(
    const SnapContext& context,
    const SnapSettings& settings,
    const std::vector<SnapCandidate>& candidates,
    const ISnapBlockTest* blockTest,
    SnapDebugReport* debugReport) const
{
    SnapResult result;

    Vector3 probePoint{};
    if (!ResolveProbePoint(context, probePoint))
    {
        result.reason = "No probe point or cursor ray available.";
        if (debugReport)
        {
            debugReport->summary = result.reason;
        }
        return result;
    }

    struct RankedCandidate
    {
        const SnapCandidate* source = nullptr;
        Vector3 resolvedPosition = Vector3(0.0f, 0.0f, 0.0f);
        float distance = 0.0f;
        float screenDistance = 0.0f;
        bool hasScreenDistance = false;
        float effectiveDistance = 0.0f;
        int priorityScore = 0;
        int toolBonus = 0;
        bool blocked = false;
        float finalScore = 0.0f;
    };

    std::vector<RankedCandidate> ranked;
    ranked.reserve(candidates.size());

    for (const SnapCandidate& candidate : candidates)
    {
        if (!IsTypeEnabled(settings, candidate.type))
        {
            if (debugReport)
            {
                SnapDecisionEntry entry;
                entry.label = candidate.debugLabel.empty() ? CandidateName(candidate.type) : candidate.debugLabel;
                entry.type = candidate.type;
                entry.accepted = false;
                entry.reason = "Rejected: candidate type disabled by active snap settings.";
                debugReport->entries.push_back(std::move(entry));
            }
            continue;
        }

        const Vector3 resolved = ResolveCandidatePosition(candidate, probePoint);
        const float distance = context.probe.hasRay
            ? DistancePointToRay(resolved, context.probe.ray)
            : Distance(probePoint, resolved);

        SnapDecisionEntry entry;
        entry.label = candidate.debugLabel.empty() ? CandidateName(candidate.type) : candidate.debugLabel;
        entry.type = candidate.type;
        entry.resolvedPosition = resolved;
        entry.distance = distance;
        if (candidate.hasScreenDistance)
        {
            entry.reason = "ScreenDistancePx=" + std::to_string(candidate.screenDistancePx);
        }

        if (candidate.hasScreenDistance &&
            settings.maxScreenSnapErrorPx > 0.0f &&
            candidate.screenDistancePx > settings.maxScreenSnapErrorPx)
        {
            entry.accepted = false;
            entry.reason = "Rejected: outside max screen snap error.";
            if (debugReport)
            {
                debugReport->entries.push_back(std::move(entry));
            }
            continue;
        }

        if (distance > settings.maxSnapDistance)
        {
            entry.accepted = false;
            entry.reason = "Rejected: outside distance threshold.";
            if (debugReport)
            {
                debugReport->entries.push_back(std::move(entry));
            }
            continue;
        }

        bool blocked = candidate.blockedHint;
        if (blockTest)
        {
            const SnapBlockTestQuery query{ probePoint, resolved, &candidate };
            blocked = blocked || blockTest->IsBlocked(query);
        }
        entry.blocked = blocked;

        if (blocked && settings.rejectBlockedCandidates)
        {
            entry.accepted = false;
            entry.reason = "Rejected: blocked.";
            if (debugReport)
            {
                debugReport->entries.push_back(std::move(entry));
            }
            continue;
        }

        RankedCandidate rankedCandidate;
        rankedCandidate.source = &candidate;
        rankedCandidate.resolvedPosition = resolved;
        rankedCandidate.distance = distance;
        rankedCandidate.hasScreenDistance = candidate.hasScreenDistance;
        rankedCandidate.screenDistance = candidate.screenDistancePx;
        rankedCandidate.effectiveDistance = blocked ? (distance + settings.blockedDistancePenalty) : distance;
        rankedCandidate.priorityScore = GetTypePriorityScore(settings, candidate.type) + candidate.priorityBias;
        rankedCandidate.toolBonus = GetToolPreferenceBonus(settings, context.activeTool, candidate.type);
        rankedCandidate.blocked = blocked;

        const float threshold = std::max(settings.maxSnapDistance, kEpsilon);
        const float normalizedDistance = Clamp01(rankedCandidate.effectiveDistance / threshold);
        const float distanceScore = (1.0f - normalizedDistance) * settings.distanceWeight;
        float screenScore = 0.0f;
        if (rankedCandidate.hasScreenDistance && settings.screenDistanceWeight > 0.0f)
        {
            const float softMax = std::max(settings.screenDistanceSoftMaxPx, 1.0f);
            const float normalizedScreenDistance = Clamp01(rankedCandidate.screenDistance / softMax);
            screenScore = (1.0f - normalizedScreenDistance) * settings.screenDistanceWeight;
        }
        const float typeScore = static_cast<float>(rankedCandidate.priorityScore) * settings.typePriorityWeight;
        const float toolScore = static_cast<float>(rankedCandidate.toolBonus) * settings.toolPreferenceWeight;
        const float blockedScore = rankedCandidate.blocked ? -settings.blockedPenaltyWeight : 0.0f;
        rankedCandidate.finalScore = distanceScore + screenScore + typeScore + toolScore + blockedScore;
        ranked.push_back(rankedCandidate);

        entry.accepted = true;
        entry.reason = blocked ? "Accepted with blocked penalty." : "Accepted.";
        if (debugReport)
        {
            debugReport->entries.push_back(std::move(entry));
        }
    }

    if (ranked.empty())
    {
        result.reason = "No valid snap candidate within threshold.";
        if (debugReport)
        {
            debugReport->summary = result.reason;
        }
        return result;
    }

    std::sort(
        ranked.begin(),
        ranked.end(),
        [&settings](const RankedCandidate& a, const RankedCandidate& b)
        {
            const float scoreDelta = std::abs(a.finalScore - b.finalScore);
            if (scoreDelta > settings.tieBreakEpsilon)
            {
                return a.finalScore > b.finalScore;
            }

            if (a.blocked != b.blocked)
            {
                return !a.blocked;
            }

            return a.distance < b.distance;
        });

    const RankedCandidate& best = ranked.front();

    const float threshold = std::max(settings.maxSnapDistance, kEpsilon);
    const float distanceFit = Clamp01(1.0f - (best.distance / threshold));
    const float blockedPenalty = best.blocked ? settings.blockedPenaltyWeight : 0.0f;
    const float confidence = std::max(0.0f, distanceFit - blockedPenalty);

    if (confidence < settings.minimumConfidence)
    {
        result.reason = "Best candidate confidence below minimum threshold.";
        result.confidence = confidence;
        if (debugReport)
        {
            debugReport->summary = result.reason;
        }
        return result;
    }

    result.didSnap = true;
    result.snapPosition = best.resolvedPosition;
    result.snapType = best.source->type;
    result.hasSourceObjectId = best.source->hasObjectId;
    result.sourceObjectId = best.source->objectId;
    result.confidence = confidence;

    std::ostringstream reason;
    reason << "Selected "
        << (best.source->debugLabel.empty() ? CandidateName(best.source->type) : best.source->debugLabel)
        << " distance=" << best.distance
        << " score=" << best.finalScore;
    if (best.blocked)
    {
        reason << " (blocked penalty applied)";
    }
    result.reason = reason.str();

    if (debugReport)
    {
        debugReport->summary = result.reason;
    }

    return result;
}
