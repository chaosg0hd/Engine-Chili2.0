#pragma once

#include "runtime/runtime_world.hpp"
#include "runtime/studio_camera.hpp"
#include "modules/render/render_types.hpp"

#include <string>
#include <vector>

namespace studio_runtime
{
    // Calibration values for picking tolerance and scoring. Expose in debug/editor
    // preferences later. All picking math reads exclusively from this struct.
    struct StudioPickingConfig
    {
        float baseSphereRadiusMultiplier = 1.0f;
        float minimumPickRadius = 0.15f;
        float screenPixelTolerance = 8.0f;
        float distanceWeight = 1.0f;
        float screenDistanceWeight = 1.5f;
        float depthWeight = 0.25f;
        bool preferClosestDepth = true;
        bool enableDebugLogging = false;
    };

    struct StudioPickRequest
    {
        int mouseX = 0;
        int mouseY = 0;
        ViewportRect viewport;
    };

    struct StudioPickCandidate
    {
        EntityId entity = 0;
        float worldDistanceToRay = 0.0f;
        float screenDistancePx = 0.0f;
        float depth = 0.0f;
        float score = 0.0f;
    };

    struct StudioPickResult
    {
        bool hit = false;
        EntityId entity = 0;
        StudioPickCandidate bestCandidate;
        std::vector<StudioPickCandidate> candidates;
        std::string debugLog;
    };

    // Picking is intentionally tolerant and weighted: editor selection should reflect
    // user intent, not brittle mathematical exactness. A candidate passes if it hits
    // the picking volume OR projects close enough to the cursor on screen. Multiple
    // candidates are scored and the best match wins.
    class StudioPickingService
    {
    public:
        StudioPickingService(const RuntimeWorld& world, const StudioCamera& camera);

        void SetConfig(const StudioPickingConfig& config);
        const StudioPickingConfig& GetConfig() const;

        StudioPickResult Pick(const StudioPickRequest& request) const;

    private:
        float ComputeEffectivePickRadius(const EntityInfo& info) const;
        float ComputeScreenDistancePx(
            const Vector3& worldPoint,
            int mouseX,
            int mouseY,
            const ViewportRect& viewport) const;
        float ComputeScore(const StudioPickCandidate& candidate) const;

        const RuntimeWorld& m_world;
        const StudioCamera& m_camera;
        StudioPickingConfig m_config;
    };
}
