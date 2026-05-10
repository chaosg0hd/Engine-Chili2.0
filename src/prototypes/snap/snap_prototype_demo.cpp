#include "snap_prototype_demo.hpp"
#include "snap_resolver.hpp"

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
    class PlaneBlockTest final : public ISnapBlockTest
    {
    public:
        PlaneBlockTest(const Vector3& pointOnPlane, const Vector3& planeNormal)
            : m_point(pointOnPlane)
            , m_normal(Normalize(planeNormal))
        {
        }

        bool IsBlocked(const SnapBlockTestQuery& query) const override
        {
            const Vector3 segment = query.candidatePoint - query.probePoint;
            const float denominator = Dot(m_normal, segment);
            if (std::abs(denominator) <= 0.000001f)
            {
                return false;
            }

            const float t = Dot(m_normal, (m_point - query.probePoint)) / denominator;
            if (t <= 0.000001f || t >= 0.999999f)
            {
                return false;
            }

            return true;
        }

    private:
        Vector3 m_point = Vector3(0.0f, 0.0f, 0.0f);
        Vector3 m_normal = Vector3(0.0f, 1.0f, 0.0f);
    };

    const char* SnapTypeToString(SnapCandidateType type)
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

    void PrintCase(
        std::ostream& out,
        const char* caseName,
        const SnapContext& context,
        const std::vector<SnapCandidate>& candidates,
        const SnapSettings& settings,
        const ISnapBlockTest* blockTest)
    {
        SnapResolver resolver;
        SnapDebugReport report;
        const SnapResult result = resolver.Resolve(context, settings, candidates, blockTest, &report);

        out << "== " << caseName << " ==\n";
        if (result.didSnap)
        {
            out << "Selected: type=" << SnapTypeToString(result.snapType)
                << " pos=(" << result.snapPosition.x << ", " << result.snapPosition.y << ", " << result.snapPosition.z << ")"
                << " confidence=" << result.confidence
                << " reason=" << result.reason << "\n";
        }
        else
        {
            out << "Selected: none"
                << " reason=" << result.reason
                << " confidence=" << result.confidence << "\n";
        }

        out << "Candidates:\n";
        for (const SnapDecisionEntry& entry : report.entries)
        {
            out << "  - [" << SnapTypeToString(entry.type) << "] " << entry.label
                << " d=" << entry.distance
                << " blocked=" << (entry.blocked ? "true" : "false")
                << " accepted=" << (entry.accepted ? "true" : "false")
                << " | " << entry.reason << "\n";
        }
        out << "\n";
    }
}

void RunSnapPrototypeDemo(std::ostream& out)
{
    std::vector<SnapCandidate> candidates;

    candidates.push_back(MakeVertexCandidate(Vector3(0.15f, 0.0f, 0.05f), 101, "Vertex A"));
    candidates.push_back(MakePointCandidate(Vector3(0.28f, 0.0f, 0.07f), 102, "Point Helper"));
    candidates.push_back(MakeObjectOriginCandidate(Vector3(0.6f, 0.0f, 0.6f), 200, "Object Origin"));

    SnapSettings settings = CreateVertexPointOriginSnapSettings();
    settings.maxSnapDistance = 0.35f;
    settings.rejectBlockedCandidates = false;
    settings.blockedDistancePenalty = 0.30f;

    PlaneBlockTest blocker(Vector3(0.32f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));

    SnapContext nearVertex;
    nearVertex.probe.hasPoint = true;
    nearVertex.probe.point = Vector3(0.16f, 0.01f, 0.06f);
    nearVertex.activeTool = SnapToolKind::Translate;
    PrintCase(out, "Near Vertex", nearVertex, candidates, settings, &blocker);

    SnapContext nearPoint;
    nearPoint.probe.hasPoint = true;
    nearPoint.probe.point = Vector3(0.285f, 0.02f, 0.06f);
    nearPoint.activeTool = SnapToolKind::Translate;
    PrintCase(out, "Near Point", nearPoint, candidates, settings, &blocker);

    SnapContext nearOrigin;
    nearOrigin.probe.hasPoint = true;
    nearOrigin.probe.point = Vector3(0.57f, 0.03f, 0.58f);
    nearOrigin.activeTool = SnapToolKind::Rotate;
    PrintCase(out, "Near Object Origin", nearOrigin, candidates, settings, &blocker);

    SnapContext farAway;
    farAway.probe.hasPoint = true;
    farAway.probe.point = Vector3(5.0f, 1.0f, 5.0f);
    farAway.activeTool = SnapToolKind::Generic;
    PrintCase(out, "Out Of Range", farAway, candidates, settings, &blocker);
}

#ifdef SNAP_PROTOTYPE_DEMO_MAIN
int main()
{
    RunSnapPrototypeDemo(std::cout);
    return 0;
}
#endif
