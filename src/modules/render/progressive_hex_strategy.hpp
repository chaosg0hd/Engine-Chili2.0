#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../prototypes/math/hex_grid.hpp"

namespace render
{
using ProgressiveHexJobFunction = std::function<void()>;
using ProgressiveHexJobSubmitter = std::function<bool(ProgressiveHexJobFunction job)>;
using ProgressiveHexJobWaiter = std::function<void()>;

struct ProgressiveHexConfig
{
    float hexSize = 0.008f;
    float centerRegionFactor = 0.40f;
    unsigned int maxPassesPerFrame = 20000U;
    unsigned int stepBatchSize = 2048U;
    double maxRenderStepSeconds = 0.002;
    double fillPassesPerSecond = 60.0;
    bool useFullResourceFill = true;
    bool persistentPaint = true;
    bool useParallelLaneOffsets = false;
    bool useRegionOwnershipColors = false;
    bool showUnfilledGrid = false;
    bool renderTerminalHexDetail = false;
    bool fadeByTimeSincePass = false;
    bool assignSubregions = true;
    bool buildFillOrder = true;
    bool highlightMajorRegionCenters = false;
    float patchSplatScale = 1.05f;
    float mainCenterStealScale = 0.0f;
    unsigned int maxSubdivisionDepthClamp = 0U;
    unsigned int mainCenterMaxSubdivisionDepthClamp = 0U;
    float mainCenterChildRadiusScale = 0.0f;
    float passFadeSeconds = 1.25f;
    float passFadeFloor = 0.14f;
};

struct ProgressiveHexPatch
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    float halfWidth = 0.0f;
    float halfHeight = 0.0f;
    float rotationRadians = 0.0f;
    std::uint32_t color = 0xFFFFFFFFU;
    bool useHexShape = true;
};

struct ProgressiveHexRegionUpdate
{
    std::string symbol;
    unsigned int passIndex = 0U;
    unsigned int depth = 0U;
    unsigned int repeatCount = 1U;
    unsigned int coveredCellCount = 0U;
    unsigned int resolvedCellCount = 0U;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    std::uint32_t color = 0xFF000000U;
};

struct ProgressiveHexStats
{
    int screenWidth = 0;
    int screenHeight = 0;
    int candidateRadius = 0;
    int majorCenterRegionRadius = 0;
    int subregionRadius = 0;
    unsigned int generatedHexCount = 0;
    unsigned int rejectedHexCount = 0;
    unsigned int regionCount = 0;
    unsigned int maxSubdivisionDepth = 0;
    unsigned int visibleFillStepCount = 0;
    unsigned int fillStepCount = 0;
    unsigned int fillLoopCycleCount = 0;
    unsigned int unassignedHexCount = 0;
    unsigned int centerPassCount = 0;
    unsigned int visibleCenterPassCount = 0;
    unsigned int lastParallelWorkerCount = 0;
    unsigned int lastParallelJobCount = 0;
    unsigned int lastParallelSampleCount = 0;
};

using ProgressiveHexSampleProvider = std::function<std::uint32_t(float screenX, float screenY, double time)>;

class ProgressiveHexStrategy
{
public:
    void Configure(const ProgressiveHexConfig& config);
    void SetSampleProvider(ProgressiveHexSampleProvider sampleProvider);
    void Resize(int screenWidth, int screenHeight, float aspectRatio);
    void Reset();
    void SetAlgorithmPreviewCycle(unsigned int cycle);
    void SetAlgorithmPreviewStep(unsigned int step, unsigned int progressCount);
    void Step(
        double totalTime,
        double deltaTime,
        double renderBudgetSeconds = -1.0,
        unsigned int workerCount = 1U,
        const ProgressiveHexJobSubmitter& submitJob = ProgressiveHexJobSubmitter{},
        const ProgressiveHexJobWaiter& waitForJobs = ProgressiveHexJobWaiter{});
    void BuildPatches(std::vector<ProgressiveHexPatch>& outPatches) const;
    void BuildRegionUpdateJobs(std::vector<ProgressiveHexRegionUpdate>& outJobs) const;
    std::string BuildAlgorithmPreviewTraversalLog() const;
    std::string BuildAlgorithmPreviewPassLog() const;
    std::string BuildAlgorithmMasterListLog() const;
    std::string BuildRuntimePassOrderLog() const;

    const ProgressiveHexConfig& GetConfig() const;
    const ProgressiveHexStats& GetStats() const;

private:
    using Hex = HexGridCoordPrototype;

    struct HexCell
    {
        Hex coord;
        float centerX = 0.0f;
        float centerY = 0.0f;
        int ring = 0;
        int regionIndex = 0;
        int subregionIndex = 0;
        int nestedSubregionIndex = 0;
        int subdivisionDepth = 0;
        int majorCenterPassIndex = -1;
        int childCenterPassIndex = -1;
        int nestedCenterPassIndex = -1;
        int fillOrderIndex = -1;
        double lastFillPassTime = -1.0;
        double lastDepthMatchedPassTime = -1.0;
        std::uint32_t resolvedColor = 0xFF000000U;
        std::array<std::uint32_t, 2> lod1DirectionalColors{};
        std::array<std::uint32_t, 7> terminalDetailColors{};
        std::array<std::uint32_t, 81> detailRasterColors{};
        std::array<bool, 81> detailRasterValid{};
        unsigned int detailRasterGridSize = 0U;
        unsigned int renderLod = 0U;
        unsigned int resolvedSampleCount = 0U;
        bool hasResolvedColor = false;
        bool hasLod1DirectionalDetail = false;
        bool hasTerminalDetail = false;
        bool hasDetailRaster = false;
        std::vector<int> path;
    };

    struct RegionWork
    {
        Hex center;
        int radius = 0;
        int depth = 0;
        unsigned int order = 0;
    };

    struct CenterPass
    {
        std::string symbol;
        char marker = '?';
        unsigned int passIndex = 0U;
        unsigned int depth = 0U;
        int regionIndex = -1;
        int subregionIndex = -1;
        int nestedSubregionIndex = -1;
        Hex center;
        Hex parent;
        Hex grandparent;
        bool hasParent = false;
        bool hasGrandparent = false;
        std::vector<int> path;
    };

private:
    std::vector<Hex> GenerateRing(const Hex& center, int radius) const;
    std::vector<Hex> GenerateSpiral(const Hex& center, int maxRadius) const;
    static std::int64_t PackHexKey(const Hex& hex);
    static Hex RoundAxial(float q, float r);
    static Hex RotateAxialOffset30Degrees(const Hex& offset);
    void Regenerate();
    int EstimateScreenCoverRadius() const;
    int ComputeMajorCenterRegionRadius() const;
    int ComputeSubregionRadius() const;
    int ComputeChildRegionRadius(int parentRadius) const;
    int ComputeChildRegionRadiusForPath(int regionIndex, const std::vector<int>& parentPath, int parentRadius) const;
    unsigned int ComputeMaxSubdivisionDepthForPath(int regionIndex, const std::vector<int>& parentPath) const;
    bool IsMainCenterLineage(int regionIndex, const std::vector<int>& path) const;
    bool CanSubdivideRegion(int parentRadius, int childRadius) const;
    int ComputeHexDistanceFromCenter(const Hex& hex) const;
    int ComputeHexDistance(const Hex& first, const Hex& second) const;
    void BuildMacroRegionCenters();
    std::vector<Hex> BuildMajorRegionChildCenters(int regionIndex) const;
    std::vector<Hex> BuildSubregionChildCenters(int regionIndex, int subregionIndex) const;
    std::vector<Hex> BuildChildRegionCenters(const Hex& parentCenter, int parentRadius, int childRadius) const;
    int FindClosestChildCenterIndex(const Hex& hex, const std::vector<Hex>& childCenters) const;
    int SelectChildCenterIndexForCell(
        const HexCell& cell,
        const Hex& parentCenter,
        int parentRadius,
        const std::vector<Hex>& childCenters) const;
    void BindMajorRegionCellsToClosestChildCenter(int regionIndex);
    void BindCellToRecursivePath(HexCell& cell) const;
    void BuildCenterPassList();
    bool HasCellsForPath(int regionIndex, const std::vector<int>& path) const;
    void AppendRecursiveCenterPasses(
        int regionIndex,
        const std::vector<int>& parentPath,
        const Hex& parentCenter,
        int parentRadius,
        unsigned int parentDepth);
    int FindCenterPassIndex(int regionIndex, int subregionIndex, int nestedSubregionIndex) const;
    void AssignCenterPassIndicesToCells();
    void BuildCenterPassConstituentLists();
    unsigned int ComputeCenterPassRepeatCount(const CenterPass& pass) const;
    int FindCenterPassIndexByPath(int regionIndex, const std::vector<int>& path) const;
    void AppendCenterPass(
        char marker,
        unsigned int depth,
        int regionIndex,
        int subregionIndex,
        int nestedSubregionIndex,
        const Hex& center,
        const Hex* parent,
        const Hex* grandparent);
    void AppendCenterPass(
        char marker,
        unsigned int depth,
        int regionIndex,
        const std::vector<int>& path,
        const Hex& center,
        const Hex* parent,
        const Hex* grandparent);
    void AssignRecursiveSubdivision(HexCell& cell, const Hex& macroCenter) const;
    void BuildTraversalFillOrder();
    std::vector<Hex> BuildTerminalRegionCells(const RegionWork& work) const;
    void AppendTerminalLeafGroupsRoundRobin(const std::vector<std::vector<Hex>>& terminalGroups);
    void AppendUnfilledCoveragePass();
    void TryMarkFillHex(const Hex& hex);
    void MarkCellFilled(std::size_t cellIndex, bool allowRepeat);
    void MarkCenterPassFilled(unsigned int centerPassIndex);
    void ProcessFillPassBatch(
        unsigned int passCount,
        unsigned int workerCount,
        const ProgressiveHexJobSubmitter& submitJob,
        const ProgressiveHexJobWaiter& waitForJobs);
    void ProcessCenterPass(const CenterPass& pass);
    void ProcessCenterPassCell(
        HexCell& cell,
        const CenterPass& pass,
        std::uint32_t inheritedColor,
        float newSampleWeight,
        bool terminalPass);
    void RenderTerminalCellDetail(HexCell& cell, float newSampleWeight);
    void RenderCellDirectionalDetail(HexCell& cell);
    void RenderCellRasterDetail(HexCell& cell, unsigned int lod);
    bool IsCellConstituentOfPass(const HexCell& cell, const CenterPass& pass) const;
    void ApplyFillPassResult(HexCell& cell, std::uint32_t color);
    void ApplyFillPassResult(HexCell& cell, std::uint32_t color, float newSampleWeight);
    static std::uint32_t BlendColors(std::uint32_t previousColor, std::uint32_t nextColor, float nextWeight);
    int ComputeHierarchyCenterDepth(const HexCell& cell) const;
    void ApplyRegionVisualDepth(int regionIndex, int visualDepth);
    bool IsMajorRegionCenter(const HexCell& cell) const;
    bool IsMajorRegionChildCenter(const HexCell& cell) const;
    bool IsNestedSubregionChildCenter(const HexCell& cell) const;
    unsigned int CountAlgorithmCenterPasses() const;
    int GetMajorCenterPassIndex(const HexCell& cell) const;
    int GetChildCenterPassIndex(const HexCell& cell) const;
    int GetNestedCenterPassIndex(const HexCell& cell) const;
    bool HasCenterPassRun(int passIndex) const;
    std::uint32_t BuildPreviewColor(int colorIndex, float scale) const;
    std::uint32_t BuildPreviewColorForCycle(unsigned int cycle, int colorIndex, float scale) const;
    std::uint32_t BuildCompletedCycleColor(const HexCell& cell, unsigned int cycle) const;
    std::uint32_t BuildMajorRegionChildOwnershipColor(const HexCell& cell) const;
    bool IsHexFullyInsideScreen(const Hex& hex) const;
    void GetHexCenter(const Hex& hex, float& outX, float& outY) const;
    void BuildCellPatch(std::vector<ProgressiveHexPatch>& outPatches, const HexCell& cell) const;
    std::uint32_t BuildRegionOwnershipColor(const HexCell& cell) const;
    std::uint32_t SampleHexColor(const Hex& hex) const;
    unsigned int ComputeRenderLod(int regionIndex, const std::vector<int>& path, const Hex& coord) const;
    unsigned int ComputePassRenderLod(const CenterPass& pass) const;
    unsigned int ComputeCellRenderLod(const HexCell& cell) const;
    static unsigned int ComputeCellRasterGridSize(unsigned int lod);
    static bool IsLocalPointInsideHex(float normalizedX, float normalizedY);
    std::uint32_t SampleCellColorWithLod(const HexCell& cell, unsigned int lod) const;
    std::uint32_t SampleCellColor(const HexCell& cell) const;
    std::uint32_t SampleDefaultGradient(float screenX) const;

private:
    ProgressiveHexConfig m_config;
    ProgressiveHexStats m_stats;
    float m_aspectRatio = 1.0f;
    float m_lastGeneratedAspectRatio = 0.0f;
    int m_lastGeneratedScreenWidth = 0;
    int m_lastGeneratedScreenHeight = 0;
    double m_totalTime = 0.0;
    double m_fillStepAccumulator = 0.0;
    unsigned int m_algorithmPreviewCycle = 0U;
    unsigned int m_algorithmPreviewStep = 0U;
    unsigned int m_algorithmPreviewProgressCount = 0U;
    std::vector<HexCell> m_hexCells;
    std::vector<Hex> m_regionCenters;
    std::vector<CenterPass> m_centerPasses;
    std::vector<std::vector<std::size_t>> m_centerPassConstituentCellIndices;
    std::vector<unsigned int> m_fillOrderCenterPassIndices;
    std::vector<std::size_t> m_fillOrderCellIndices;
    std::unordered_map<std::int64_t, std::size_t> m_hexCellIndexByCoord;
    std::vector<int> m_regionVisualDepths;
    ProgressiveHexSampleProvider m_sampleProvider;
};
}
