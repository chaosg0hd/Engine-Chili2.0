#include "progressive_hex_strategy.hpp"

#include "../../prototypes/math/color_sample_math.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <utility>

namespace render
{
namespace
{
constexpr float Pi = 3.14159265358979323846f;
constexpr float Sqrt3 = 1.73205080756887729353f;
constexpr float ScreenEdgeEpsilon = 0.0001f;
constexpr std::size_t TerminalDetailSampleCount = 7U;
constexpr float TerminalDetailOffsetScale = 0.8660254f;
constexpr float TerminalDetailOffsets[TerminalDetailSampleCount][2] =
{
    {  0.0f,                       0.0f },
    {  1.0f,                       0.0f },
    { -1.0f,                       0.0f },
    {  0.5f,  TerminalDetailOffsetScale },
    { -0.5f,  TerminalDetailOffsetScale },
    {  0.5f, -TerminalDetailOffsetScale },
    { -0.5f, -TerminalDetailOffsetScale }
};
constexpr unsigned int MaxCellRasterGridSize = 9U;
constexpr std::size_t Lod1SampleCount = 2U;
constexpr std::size_t Lod2SampleCount = 3U;
constexpr std::size_t Lod3SampleCount = 7U;
constexpr std::size_t Lod4SampleCount = 13U;
constexpr float Lod1SampleOffsets[Lod1SampleCount][2] =
{
    { -0.45f,  0.0f },
    {  0.45f,  0.0f }
};
constexpr float Lod2SampleOffsets[Lod2SampleCount][2] =
{
    {  0.0f,  0.0f },
    { -0.6f,  0.0f },
    {  0.6f,  0.0f }
};
constexpr float Lod3SampleOffsets[Lod3SampleCount][2] =
{
    {  0.0f,                       0.0f },
    {  1.0f,                       0.0f },
    { -1.0f,                       0.0f },
    {  0.5f,  TerminalDetailOffsetScale },
    { -0.5f,  TerminalDetailOffsetScale },
    {  0.5f, -TerminalDetailOffsetScale },
    { -0.5f, -TerminalDetailOffsetScale }
};
constexpr float Lod4SampleOffsets[Lod4SampleCount][2] =
{
    {  0.0f,                       0.0f },
    {  0.6f,                       0.0f },
    { -0.6f,                       0.0f },
    {  0.3f,  TerminalDetailOffsetScale * 0.6f },
    { -0.3f,  TerminalDetailOffsetScale * 0.6f },
    {  0.3f, -TerminalDetailOffsetScale * 0.6f },
    { -0.3f, -TerminalDetailOffsetScale * 0.6f },
    {  1.0f,                       0.0f },
    { -1.0f,                       0.0f },
    {  0.5f,  TerminalDetailOffsetScale },
    { -0.5f,  TerminalDetailOffsetScale },
    {  0.5f, -TerminalDetailOffsetScale },
    { -0.5f, -TerminalDetailOffsetScale }
};

float Clamp01(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

std::uint32_t GetMajorRegionBaseColor(int regionIndex)
{
    static constexpr std::array<std::uint32_t, 12> regionPalette =
    {{
        0xFFE84D5BU,
        0xFF2A9D8FU,
        0xFFF4A261U,
        0xFF5E60CEU,
        0xFF55A630U,
        0xFFE76F51U,
        0xFF00B4D8U,
        0xFFD81159U,
        0xFF8AC926U,
        0xFFFFCA3AU,
        0xFF1982C4U,
        0xFF6A4C93U
    }};

    if (regionIndex < 0)
    {
        return 0xFF343A40U;
    }

    return regionPalette[static_cast<std::size_t>(regionIndex) % regionPalette.size()];
}

std::string BuildSymbolForIndex(int index)
{
    if (index < 0)
    {
        return "-";
    }

    std::string symbol;
    int value = index;
    do
    {
        const char digit = static_cast<char>('A' + (value % 26));
        symbol.insert(symbol.begin(), digit);
        value = (value / 26) - 1;
    }
    while (value >= 0);

    return symbol;
}

std::string BuildSymbolPath(int regionIndex, const std::vector<int>& childPath)
{
    std::string path = BuildSymbolForIndex(regionIndex);
    for (int childIndex : childPath)
    {
        path += "|" + BuildSymbolForIndex(childIndex);
    }

    return path;
}
}

void ProgressiveHexStrategy::Configure(const ProgressiveHexConfig& config)
{
    m_config = config;
    if (m_config.hexSize <= 0.0001f)
    {
        m_config.hexSize = 0.0001f;
    }
    if (m_config.centerRegionFactor <= 0.0f)
    {
        m_config.centerRegionFactor = 0.01f;
    }
    if (m_config.maxPassesPerFrame == 0U)
    {
        m_config.maxPassesPerFrame = 1U;
    }
    if (m_config.stepBatchSize == 0U)
    {
        m_config.stepBatchSize = 1U;
    }
    if (m_config.patchSplatScale <= 0.01f)
    {
        m_config.patchSplatScale = 0.01f;
    }
    m_config.mainCenterStealScale = std::max(0.0f, m_config.mainCenterStealScale);
    m_config.mainCenterChildRadiusScale = std::max(0.0f, m_config.mainCenterChildRadiusScale);
    if (m_config.passFadeSeconds <= 0.01f)
    {
        m_config.passFadeSeconds = 0.01f;
    }
    m_config.passFadeFloor = std::max(0.0f, std::min(1.0f, m_config.passFadeFloor));
    Reset();
}

void ProgressiveHexStrategy::SetSampleProvider(ProgressiveHexSampleProvider sampleProvider)
{
    m_sampleProvider = std::move(sampleProvider);
}

void ProgressiveHexStrategy::SetAlgorithmPreviewCycle(unsigned int cycle)
{
    m_algorithmPreviewCycle = cycle;
}

void ProgressiveHexStrategy::Resize(int screenWidth, int screenHeight, float aspectRatio)
{
    m_stats.screenWidth = screenWidth;
    m_stats.screenHeight = screenHeight;
    m_aspectRatio = aspectRatio > 0.001f ? aspectRatio : 1.0f;

    if (m_hexCells.empty() ||
        std::fabs(m_aspectRatio - m_lastGeneratedAspectRatio) > 0.001f ||
        m_stats.screenWidth != m_lastGeneratedScreenWidth ||
        m_stats.screenHeight != m_lastGeneratedScreenHeight)
    {
        Regenerate();
    }
}

void ProgressiveHexStrategy::Reset()
{
    m_hexCells.clear();
    m_regionCenters.clear();
    m_centerPasses.clear();
    m_centerPassConstituentCellIndices.clear();
    m_fillOrderCenterPassIndices.clear();
    m_fillOrderCellIndices.clear();
    m_hexCellIndexByCoord.clear();
    m_regionVisualDepths.clear();
    m_fillStepAccumulator = 0.0;
    m_stats = ProgressiveHexStats{};
}

void ProgressiveHexStrategy::SetAlgorithmPreviewStep(unsigned int step, unsigned int progressCount)
{
    m_algorithmPreviewStep = step;
    m_algorithmPreviewProgressCount = progressCount;
    m_stats.visibleCenterPassCount = (step > 0U)
        ? std::min(progressCount, m_stats.centerPassCount)
        : 0U;
}

void ProgressiveHexStrategy::Step(
    double totalTime,
    double deltaTime,
    double renderBudgetSeconds,
    unsigned int workerCount,
    const ProgressiveHexJobSubmitter& submitJob,
    const ProgressiveHexJobWaiter& waitForJobs)
{
    m_totalTime = totalTime;
    m_stats.lastParallelWorkerCount = 0U;
    m_stats.lastParallelJobCount = 0U;
    m_stats.lastParallelSampleCount = 0U;
    if (m_fillOrderCellIndices.empty() && m_fillOrderCenterPassIndices.empty())
    {
        return;
    }

    unsigned int passCount = m_config.maxPassesPerFrame;
    if (!m_config.useFullResourceFill)
    {
        m_fillStepAccumulator += deltaTime * m_config.fillPassesPerSecond;
        passCount = static_cast<unsigned int>(m_fillStepAccumulator);
    }
    else
    {
        passCount = std::min(passCount, m_stats.fillStepCount);
    }

    if (passCount == 0U)
    {
        return;
    }

    passCount = std::min(passCount, m_config.maxPassesPerFrame);
    if (!m_config.useFullResourceFill)
    {
        m_fillStepAccumulator -= static_cast<double>(passCount);
    }

    const auto stepStart = std::chrono::steady_clock::now();
    unsigned int processedPassCount = 0U;
    while (processedPassCount < passCount)
    {
        const unsigned int remainingPassCount = passCount - processedPassCount;
        const unsigned int contiguousPassCount = std::min(
            std::min(remainingPassCount, m_config.stepBatchSize),
            m_stats.fillStepCount - m_stats.visibleFillStepCount);
        if (contiguousPassCount == 0U)
        {
            break;
        }

        ProcessFillPassBatch(contiguousPassCount, workerCount, submitJob, waitForJobs);
        processedPassCount += contiguousPassCount;

        const double activeBudgetSeconds =
            (renderBudgetSeconds >= 0.0) ? renderBudgetSeconds : m_config.maxRenderStepSeconds;
        if (activeBudgetSeconds > 0.0)
        {
            const std::chrono::duration<double> spent = std::chrono::steady_clock::now() - stepStart;
            if (spent.count() >= activeBudgetSeconds)
            {
                break;
            }
        }
    }
}

void ProgressiveHexStrategy::BuildPatches(std::vector<ProgressiveHexPatch>& outPatches) const
{
    for (const HexCell& cell : m_hexCells)
    {
        BuildCellPatch(outPatches, cell);
    }
}

void ProgressiveHexStrategy::BuildRegionUpdateJobs(std::vector<ProgressiveHexRegionUpdate>& outJobs) const
{
    outJobs.clear();
    outJobs.reserve(m_centerPasses.size());

    std::vector<bool> emitted(m_centerPasses.size(), false);
    const auto appendJob =
        [this, &outJobs, &emitted](unsigned int centerPassIndex)
        {
            if (centerPassIndex >= m_centerPasses.size() || emitted[centerPassIndex])
            {
                return;
            }

            const CenterPass& pass = m_centerPasses[centerPassIndex];
            ProgressiveHexRegionUpdate job;
            job.symbol = pass.symbol;
            job.passIndex = pass.passIndex;
            job.depth = pass.depth;
            job.repeatCount = ComputeCenterPassRepeatCount(pass);
            GetHexCenter(pass.center, job.centerX, job.centerY);

            bool hasBounds = false;
            std::uint64_t redTotal = 0U;
            std::uint64_t greenTotal = 0U;
            std::uint64_t blueTotal = 0U;
            if (centerPassIndex < m_centerPassConstituentCellIndices.size())
            {
                const std::vector<std::size_t>& constituents = m_centerPassConstituentCellIndices[centerPassIndex];
                job.coveredCellCount = static_cast<unsigned int>(constituents.size());
                for (std::size_t cellIndex : constituents)
                {
                    if (cellIndex >= m_hexCells.size())
                    {
                        continue;
                    }

                    const HexCell& cell = m_hexCells[cellIndex];
                    if (cell.hasResolvedColor)
                    {
                        redTotal += (cell.resolvedColor >> 16U) & 0xFFU;
                        greenTotal += (cell.resolvedColor >> 8U) & 0xFFU;
                        blueTotal += cell.resolvedColor & 0xFFU;
                        ++job.resolvedCellCount;
                    }

                    float cellX = 0.0f;
                    float cellY = 0.0f;
                    GetHexCenter(cell.coord, cellX, cellY);
                    const float halfWidth = (m_config.hexSize * m_config.patchSplatScale) / m_aspectRatio;
                    const float halfHeight = m_config.hexSize * m_config.patchSplatScale;
                    const float minX = cellX - halfWidth;
                    const float minY = cellY - halfHeight;
                    const float maxX = cellX + halfWidth;
                    const float maxY = cellY + halfHeight;
                    if (!hasBounds)
                    {
                        job.minX = minX;
                        job.minY = minY;
                        job.maxX = maxX;
                        job.maxY = maxY;
                        hasBounds = true;
                    }
                    else
                    {
                        job.minX = std::min(job.minX, minX);
                        job.minY = std::min(job.minY, minY);
                        job.maxX = std::max(job.maxX, maxX);
                        job.maxY = std::max(job.maxY, maxY);
                    }
                }
            }

            if (job.resolvedCellCount > 0U)
            {
                const std::uint32_t red = static_cast<std::uint32_t>(redTotal / job.resolvedCellCount);
                const std::uint32_t green = static_cast<std::uint32_t>(greenTotal / job.resolvedCellCount);
                const std::uint32_t blue = static_cast<std::uint32_t>(blueTotal / job.resolvedCellCount);
                job.color = 0xFF000000U | (red << 16U) | (green << 8U) | blue;
            }

            if (!hasBounds)
            {
                const float halfWidth = (m_config.hexSize * m_config.patchSplatScale) / m_aspectRatio;
                const float halfHeight = m_config.hexSize * m_config.patchSplatScale;
                job.minX = job.centerX - halfWidth;
                job.minY = job.centerY - halfHeight;
                job.maxX = job.centerX + halfWidth;
                job.maxY = job.centerY + halfHeight;
            }

            outJobs.push_back(std::move(job));
            emitted[centerPassIndex] = true;
        };

    for (unsigned int centerPassIndex : m_fillOrderCenterPassIndices)
    {
        appendJob(centerPassIndex);
    }

    for (std::size_t index = 0; index < m_centerPasses.size(); ++index)
    {
        appendJob(static_cast<unsigned int>(index));
    }
}

std::string ProgressiveHexStrategy::BuildAlgorithmPreviewTraversalLog() const
{
    std::ostringstream log;
    log << "Progressive hex algorithm traversal log\n";
    log << "screen=" << m_stats.screenWidth << "x" << m_stats.screenHeight
        << " aspect=" << m_aspectRatio << "\n";
    log << "hex_size=" << m_config.hexSize
        << " candidate_radius=" << m_stats.candidateRadius
        << " major_region_radius=" << m_stats.majorCenterRegionRadius
        << " child_radius=" << ComputeChildRegionRadius(m_stats.majorCenterRegionRadius)
        << "\n\n";

    log << std::fixed << std::setprecision(3);
    log << "[major_region_centers]\n";
    log << "index,q,r,polar_radius,angle_degrees,screen_x,screen_y\n";
    for (std::size_t index = 0; index < m_regionCenters.size(); ++index)
    {
        const Hex& center = m_regionCenters[index];
        float screenX = 0.0f;
        float screenY = 0.0f;
        GetHexCenter(center, screenX, screenY);
        const float polarRadius = std::sqrt((screenX * screenX) + (screenY * screenY));
        const float angleDegrees = std::atan2(screenY, screenX) * (180.0f / Pi);
        log << index << ','
            << center.q << ','
            << center.r << ','
            << polarRadius << ','
            << angleDegrees << ','
            << screenX << ','
            << screenY << '\n';
    }

    log << "\n[center_major_region_child_centers]\n";
    log << "index,q,r,relative_q,relative_r,polar_radius_hex,angle_degrees,screen_x,screen_y\n";
    if (!m_regionCenters.empty())
    {
        const Hex& parentCenter = m_regionCenters[0];
        const std::vector<Hex> childCenters = BuildMajorRegionChildCenters(0);

        for (std::size_t index = 0; index < childCenters.size(); ++index)
        {
            const Hex& center = childCenters[index];
            const Hex relative{ center.q - parentCenter.q, center.r - parentCenter.r };
            const float x = 1.5f * static_cast<float>(relative.q);
            const float y = Sqrt3 * (static_cast<float>(relative.r) + (static_cast<float>(relative.q) * 0.5f));
            const float polarRadius = std::sqrt((x * x) + (y * y));
            const float angleDegrees = std::atan2(y, x) * (180.0f / Pi);
            float screenX = 0.0f;
            float screenY = 0.0f;
            GetHexCenter(center, screenX, screenY);
            log << index << ','
                << center.q << ','
                << center.r << ','
                << relative.q << ','
                << relative.r << ','
                << polarRadius << ','
                << angleDegrees << ','
                << screenX << ','
                << screenY << '\n';
        }
    }

    return log.str();
}

std::string ProgressiveHexStrategy::BuildAlgorithmPreviewPassLog() const
{
    return BuildAlgorithmMasterListLog();
}

std::string ProgressiveHexStrategy::BuildAlgorithmMasterListLog() const
{
    const auto appendCenter =
        [this](std::ostringstream& log, const CenterPass& pass)
        {
            const float planeX = 1.5f * static_cast<float>(pass.center.q);
            const float planeY = Sqrt3 * (static_cast<float>(pass.center.r) + (static_cast<float>(pass.center.q) * 0.5f));
            const float polarRadius = std::sqrt((planeX * planeX) + (planeY * planeY));
            const float angleDegrees = std::atan2(planeY, planeX) * (180.0f / Pi);

            log << pass.symbol << ','
                << pass.passIndex << ','
                << pass.marker << ','
                << pass.depth << ','
                << ComputePassRenderLod(pass) << ','
                << pass.center.q << ','
                << pass.center.r << ','
                << polarRadius << ','
                << angleDegrees << ',';

            if (pass.hasParent)
            {
                log << pass.parent.q << ':' << pass.parent.r;
            }
            else
            {
                log << "none";
            }

            log << ',';
            if (pass.hasGrandparent)
            {
                log << pass.grandparent.q << ':' << pass.grandparent.r;
            }
            else
            {
                log << "none";
            }

            log << '\n';
        };

    std::ostringstream log;
    log << "Progressive hex master center list\n";
    log << "total_passes=" << m_centerPasses.size()
        << " generated_cells=" << m_stats.generatedHexCount
        << " major_regions=" << m_stats.regionCount
        << " max_depth=" << m_stats.maxSubdivisionDepth << "\n";
    log << "symbol,pass_index,marker,depth,lod,q,r,polar_radius,angle_degrees,parent_qr,grandparent_qr\n";

    for (std::size_t passIndex = 0; passIndex < m_centerPasses.size(); ++passIndex)
    {
        appendCenter(log, m_centerPasses[passIndex]);
    }

    return log.str();
}

std::string ProgressiveHexStrategy::BuildRuntimePassOrderLog() const
{
    std::ostringstream log;
    log << "Progressive hex runtime pass order\n";
    log << "fill_step_count=" << m_stats.fillStepCount
        << " center_pass_count=" << m_stats.centerPassCount
        << " max_depth=" << m_stats.maxSubdivisionDepth
        << " bias=n^2_main_center_lineage_multiplier order=weighted_phase\n";

    std::vector<unsigned int> depthSlotCounts(static_cast<std::size_t>(m_stats.maxSubdivisionDepth) + 1U, 0U);
    for (unsigned int centerPassIndex : m_fillOrderCenterPassIndices)
    {
        if (centerPassIndex >= m_centerPasses.size())
        {
            continue;
        }

        const unsigned int depth = m_centerPasses[centerPassIndex].depth;
        if (depth >= depthSlotCounts.size())
        {
            depthSlotCounts.resize(static_cast<std::size_t>(depth) + 1U, 0U);
        }
        ++depthSlotCounts[depth];
    }

    log << "depth,slot_count,expected_repeat_per_pass\n";
    for (std::size_t depth = 0U; depth < depthSlotCounts.size(); ++depth)
    {
        CenterPass syntheticPass;
        syntheticPass.depth = static_cast<unsigned int>(depth);
        log << depth << ","
            << depthSlotCounts[depth] << ","
            << ComputeCenterPassRepeatCount(syntheticPass) << "\n";
    }

    log << "\nslot_index,pass_index,symbol,depth,lod,repeat_ordinal,repeat_count,phase,q,r,constituent_count,previous_same_pass_gap,previous_same_depth_gap\n";
    std::vector<unsigned int> passSeenCounts(m_centerPasses.size(), 0U);
    std::vector<int> lastSlotByPass(m_centerPasses.size(), -1);
    std::vector<int> lastSlotByDepth(depthSlotCounts.size(), -1);
    for (std::size_t slotIndex = 0U; slotIndex < m_fillOrderCenterPassIndices.size(); ++slotIndex)
    {
        const unsigned int centerPassIndex = m_fillOrderCenterPassIndices[slotIndex];
        if (centerPassIndex >= m_centerPasses.size())
        {
            continue;
        }

        const CenterPass& pass = m_centerPasses[centerPassIndex];
        if (pass.depth >= lastSlotByDepth.size())
        {
            lastSlotByDepth.resize(static_cast<std::size_t>(pass.depth) + 1U, -1);
        }

        const unsigned int repeatOrdinal = passSeenCounts[centerPassIndex]++;
        const unsigned int repeatCount = ComputeCenterPassRepeatCount(pass);
        const float phase = repeatCount > 0U
            ? (static_cast<float>(repeatOrdinal) + 0.5f) / static_cast<float>(repeatCount)
            : 0.0f;
        const int previousPassSlot = lastSlotByPass[centerPassIndex];
        const int previousDepthSlot = lastSlotByDepth[pass.depth];
        const int samePassGap = previousPassSlot >= 0
            ? static_cast<int>(slotIndex) - previousPassSlot
            : -1;
        const int sameDepthGap = previousDepthSlot >= 0
            ? static_cast<int>(slotIndex) - previousDepthSlot
            : -1;
        const unsigned int constituentCount =
            centerPassIndex < m_centerPassConstituentCellIndices.size()
                ? static_cast<unsigned int>(m_centerPassConstituentCellIndices[centerPassIndex].size())
                : 0U;

        log << slotIndex << ","
            << pass.passIndex << ","
            << pass.symbol << ","
            << pass.depth << ","
            << ComputePassRenderLod(pass) << ","
            << repeatOrdinal << ","
            << repeatCount << ","
            << phase << ","
            << pass.center.q << ","
            << pass.center.r << ","
            << constituentCount << ","
            << samePassGap << ","
            << sameDepthGap << "\n";

        lastSlotByPass[centerPassIndex] = static_cast<int>(slotIndex);
        lastSlotByDepth[pass.depth] = static_cast<int>(slotIndex);
    }

    return log.str();
}

const ProgressiveHexConfig& ProgressiveHexStrategy::GetConfig() const
{
    return m_config;
}

const ProgressiveHexStats& ProgressiveHexStrategy::GetStats() const
{
    return m_stats;
}

std::vector<ProgressiveHexStrategy::Hex> ProgressiveHexStrategy::GenerateRing(
    const Hex& center,
    int radius) const
{
    static constexpr std::array<Hex, 6> directions =
    {{
        { 1, 0 },
        { 1, -1 },
        { 0, -1 },
        { -1, 0 },
        { -1, 1 },
        { 0, 1 }
    }};

    std::vector<Hex> result;
    if (radius == 0)
    {
        result.push_back(center);
        return result;
    }

    result.reserve(static_cast<std::size_t>(radius * 6));
    Hex hex =
    {
        center.q + (directions[4].q * radius),
        center.r + (directions[4].r * radius)
    };

    for (int side = 0; side < 6; ++side)
    {
        for (int step = 0; step < radius; ++step)
        {
            result.push_back(hex);
            hex.q += directions[side].q;
            hex.r += directions[side].r;
        }
    }

    return result;
}

std::vector<ProgressiveHexStrategy::Hex> ProgressiveHexStrategy::GenerateSpiral(
    const Hex& center,
    int maxRadius) const
{
    std::vector<Hex> result;
    for (int radius = 0; radius <= maxRadius; ++radius)
    {
        std::vector<Hex> ring = GenerateRing(center, radius);
        result.insert(result.end(), ring.begin(), ring.end());
    }
    return result;
}

std::int64_t ProgressiveHexStrategy::PackHexKey(const Hex& hex)
{
    return PackHexGridKey(hex);
}

ProgressiveHexStrategy::Hex ProgressiveHexStrategy::RoundAxial(float q, float r)
{
    return RoundHexGridAxial(q, r);
}

ProgressiveHexStrategy::Hex ProgressiveHexStrategy::RotateAxialOffset30Degrees(const Hex& offset)
{
    return RotateHexGridOffset30Degrees(offset);
}

void ProgressiveHexStrategy::Regenerate()
{
    m_hexCells.clear();
    m_regionCenters.clear();
    m_centerPasses.clear();
    m_centerPassConstituentCellIndices.clear();
    m_fillOrderCenterPassIndices.clear();
    m_fillOrderCellIndices.clear();
    m_hexCellIndexByCoord.clear();

    m_lastGeneratedAspectRatio = m_aspectRatio;
    m_lastGeneratedScreenWidth = m_stats.screenWidth;
    m_lastGeneratedScreenHeight = m_stats.screenHeight;
    m_stats.candidateRadius = EstimateScreenCoverRadius();
    m_stats.majorCenterRegionRadius = ComputeMajorCenterRegionRadius();
    m_stats.subregionRadius = ComputeSubregionRadius();
    m_stats.visibleFillStepCount = 0;
    m_stats.fillStepCount = 0;
    m_fillStepAccumulator = 0.0;
    m_stats.fillLoopCycleCount = 0;
    m_stats.rejectedHexCount = 0;
    m_stats.regionCount = 0;
    m_stats.maxSubdivisionDepth = 0;
    m_stats.unassignedHexCount = 0;
    m_stats.centerPassCount = 0;
    m_stats.visibleCenterPassCount = 0;

    BuildMacroRegionCenters();
    m_stats.regionCount = static_cast<unsigned int>(m_regionCenters.size());
    m_regionVisualDepths.assign(m_regionCenters.size(), -1);

    for (int radius = 0; radius <= m_stats.candidateRadius; ++radius)
    {
        const std::vector<Hex> ring = GenerateRing({ 0, 0 }, radius);
        for (const Hex& hex : ring)
        {
            if (!IsHexFullyInsideScreen(hex))
            {
                ++m_stats.rejectedHexCount;
                continue;
            }

            HexCell cell;
            cell.coord = hex;
            GetHexCenter(hex, cell.centerX, cell.centerY);
            cell.ring = radius;
            cell.regionIndex = -1;
            cell.subregionIndex = -1;
            cell.nestedSubregionIndex = -1;
            cell.majorCenterPassIndex = -1;
            cell.childCenterPassIndex = -1;
            cell.nestedCenterPassIndex = -1;

            int nearestDistance = 1000000;
            for (std::size_t regionIndex = 0; regionIndex < m_regionCenters.size(); ++regionIndex)
            {
                const int distanceToRegion = ComputeHexDistance(hex, m_regionCenters[regionIndex]);
                if (distanceToRegion < nearestDistance)
                {
                    nearestDistance = distanceToRegion;
                    cell.regionIndex = static_cast<int>(regionIndex);
                }
            }

            if (cell.regionIndex < 0)
            {
                ++m_stats.unassignedHexCount;
            }
            else if (m_config.assignSubregions)
            {
                AssignRecursiveSubdivision(
                    cell,
                    m_regionCenters[static_cast<std::size_t>(cell.regionIndex)]);
                m_stats.maxSubdivisionDepth = std::max(
                    m_stats.maxSubdivisionDepth,
                    static_cast<unsigned int>(cell.subdivisionDepth));
            }

            const std::size_t cellIndex = m_hexCells.size();
            m_hexCellIndexByCoord[PackHexKey(cell.coord)] = cellIndex;
            m_hexCells.push_back(cell);
        }
    }

    m_stats.generatedHexCount = static_cast<unsigned int>(m_hexCells.size());
    for (std::size_t regionIndex = 0; regionIndex < m_regionCenters.size(); ++regionIndex)
    {
        BindMajorRegionCellsToClosestChildCenter(static_cast<int>(regionIndex));
    }
    BuildCenterPassList();
    AssignCenterPassIndicesToCells();
    for (HexCell& cell : m_hexCells)
    {
        cell.renderLod = ComputeRenderLod(cell.regionIndex, cell.path, cell.coord);
    }
    BuildCenterPassConstituentLists();
    m_stats.centerPassCount = static_cast<unsigned int>(m_centerPasses.size());
    if (m_config.buildFillOrder)
    {
        BuildTraversalFillOrder();
        if (m_fillOrderCenterPassIndices.empty())
        {
            AppendUnfilledCoveragePass();
        }
    }
}

int ProgressiveHexStrategy::EstimateScreenCoverRadius() const
{
    const float horizontalRadius = (m_aspectRatio / (m_config.hexSize * 1.5f)) + 2.0f;
    const float verticalRadius = (1.0f / (m_config.hexSize * Sqrt3)) + 2.0f;
    return std::max(1, static_cast<int>(std::ceil(std::max(horizontalRadius, verticalRadius))));
}

int ProgressiveHexStrategy::ComputeMajorCenterRegionRadius() const
{
    const int baseSize = std::min(m_stats.screenWidth, m_stats.screenHeight);
    if (baseSize <= 0)
    {
        return 1;
    }

    const float centerPixelRadius = static_cast<float>(baseSize) * m_config.centerRegionFactor;
    const float hexPixelSize = m_config.hexSize * static_cast<float>(baseSize) * 0.5f;
    const float ringStep = 1.5f * hexPixelSize;
    if (ringStep <= 0.001f)
    {
        return 1;
    }

    return std::max(1, static_cast<int>(std::floor(centerPixelRadius / ringStep)));
}

int ProgressiveHexStrategy::ComputeSubregionRadius() const
{
    return ComputeChildRegionRadius(m_stats.majorCenterRegionRadius);
}

int ProgressiveHexStrategy::ComputeChildRegionRadius(int parentRadius) const
{
    return std::max(1, parentRadius / 2);
}

bool ProgressiveHexStrategy::IsMainCenterLineage(int regionIndex, const std::vector<int>& path) const
{
    if (regionIndex != 0)
    {
        return false;
    }

    for (int childIndex : path)
    {
        if (childIndex != 0)
        {
            return false;
        }
    }

    return true;
}

unsigned int ProgressiveHexStrategy::ComputeMaxSubdivisionDepthForPath(
    int regionIndex,
    const std::vector<int>& parentPath) const
{
    if (IsMainCenterLineage(regionIndex, parentPath) &&
        m_config.mainCenterMaxSubdivisionDepthClamp > 0U)
    {
        return m_config.mainCenterMaxSubdivisionDepthClamp;
    }

    return m_config.maxSubdivisionDepthClamp;
}

int ProgressiveHexStrategy::ComputeChildRegionRadiusForPath(
    int regionIndex,
    const std::vector<int>& parentPath,
    int parentRadius) const
{
    if (IsMainCenterLineage(regionIndex, parentPath) &&
        m_config.mainCenterChildRadiusScale > 0.0f)
    {
        const int scaledRadius = static_cast<int>(
            std::round(static_cast<float>(parentRadius) * m_config.mainCenterChildRadiusScale));
        return std::max(1, std::min(parentRadius - 1, scaledRadius));
    }

    return ComputeChildRegionRadius(parentRadius);
}

bool ProgressiveHexStrategy::CanSubdivideRegion(int parentRadius, int childRadius) const
{
    return parentRadius >= 2 && childRadius > 0 && childRadius < parentRadius;
}

int ProgressiveHexStrategy::ComputeHexDistanceFromCenter(const Hex& hex) const
{
    return ComputeHexDistance(hex, { 0, 0 });
}

int ProgressiveHexStrategy::ComputeHexDistance(const Hex& first, const Hex& second) const
{
    return ComputeHexGridDistance(first, second);
}

void ProgressiveHexStrategy::BuildMacroRegionCenters()
{
    const int regionSpacing = std::max(1, (m_stats.majorCenterRegionRadius * 2) + 1);
    const int macroRadius = std::max(1, static_cast<int>(
        std::ceil(static_cast<float>(m_stats.candidateRadius) / static_cast<float>(regionSpacing))) + 1);

    for (int radius = 0; radius <= macroRadius; ++radius)
    {
        const std::vector<Hex> ring = GenerateRing({ 0, 0 }, radius);
        for (const Hex& macroHex : ring)
        {
            m_regionCenters.push_back({ macroHex.q * regionSpacing, macroHex.r * regionSpacing });
        }
    }
}

std::vector<ProgressiveHexStrategy::Hex> ProgressiveHexStrategy::BuildMajorRegionChildCenters(
    int regionIndex) const
{
    if (regionIndex < 0 || static_cast<std::size_t>(regionIndex) >= m_regionCenters.size())
    {
        return {};
    }

    const int childRadius = ComputeChildRegionRadius(m_stats.majorCenterRegionRadius);
    return BuildChildRegionCenters(
        m_regionCenters[static_cast<std::size_t>(regionIndex)],
        m_stats.majorCenterRegionRadius,
        childRadius);
}

std::vector<ProgressiveHexStrategy::Hex> ProgressiveHexStrategy::BuildSubregionChildCenters(
    int regionIndex,
    int subregionIndex) const
{
    const std::vector<Hex> childCenters = BuildMajorRegionChildCenters(regionIndex);
    if (subregionIndex < 0 || static_cast<std::size_t>(subregionIndex) >= childCenters.size())
    {
        return {};
    }

    const int childRadius = ComputeChildRegionRadius(m_stats.majorCenterRegionRadius);
    const int nestedChildRadius = ComputeChildRegionRadius(childRadius);
    return BuildChildRegionCenters(
        childCenters[static_cast<std::size_t>(subregionIndex)],
        childRadius,
        nestedChildRadius);
}

std::vector<ProgressiveHexStrategy::Hex> ProgressiveHexStrategy::BuildChildRegionCenters(
    const Hex& parentCenter,
    int parentRadius,
    int childRadius) const
{
    std::vector<Hex> centers;
    const int childSpacing = std::max(1, childRadius);
    const int outerRadius = std::max(1, static_cast<int>(
        std::round(static_cast<float>(parentRadius) * 0.75f)));
    const bool includeOuterRing = parentRadius >= 6 && outerRadius > childSpacing;
    const std::vector<Hex> directions = GenerateRing({ 0, 0 }, 1);

    centers.reserve((directions.size() * (includeOuterRing ? 2U : 1U)) + 1U);
    centers.push_back(parentCenter);

    for (const Hex& direction : directions)
    {
        centers.push_back({ parentCenter.q + (direction.q * childSpacing), parentCenter.r + (direction.r * childSpacing) });
    }

    if (includeOuterRing)
    {
        for (const Hex& direction : directions)
        {
            const Hex outerOffset = RotateAxialOffset30Degrees({ direction.q * outerRadius, direction.r * outerRadius });
            centers.push_back({ parentCenter.q + outerOffset.q, parentCenter.r + outerOffset.r });
        }
    }

    return centers;
}

int ProgressiveHexStrategy::FindClosestChildCenterIndex(const Hex& hex, const std::vector<Hex>& childCenters) const
{
    int nearestDistance = 1000000;
    int nearestChildIndex = -1;
    for (std::size_t childIndex = 0; childIndex < childCenters.size(); ++childIndex)
    {
        const int distance = ComputeHexDistance(hex, childCenters[childIndex]);
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestChildIndex = static_cast<int>(childIndex);
        }
    }

    return nearestChildIndex;
}

int ProgressiveHexStrategy::SelectChildCenterIndexForCell(
    const HexCell& cell,
    const Hex& parentCenter,
    int parentRadius,
    const std::vector<Hex>& childCenters) const
{
    if (m_config.mainCenterStealScale > 0.0f &&
        cell.regionIndex == 0 &&
        !childCenters.empty())
    {
        bool onMainCenterLineage = true;
        for (int pathIndex : cell.path)
        {
            if (pathIndex != 0)
            {
                onMainCenterLineage = false;
                break;
            }
        }

        const int stealRadius = std::max(
            1,
            static_cast<int>(std::round(static_cast<float>(parentRadius) * m_config.mainCenterStealScale)));
        if (onMainCenterLineage && ComputeHexDistance(cell.coord, parentCenter) <= stealRadius)
        {
            return 0;
        }
    }

    return FindClosestChildCenterIndex(cell.coord, childCenters);
}

void ProgressiveHexStrategy::BindMajorRegionCellsToClosestChildCenter(int regionIndex)
{
    for (HexCell& cell : m_hexCells)
    {
        if (cell.regionIndex != regionIndex)
        {
            continue;
        }

        BindCellToRecursivePath(cell);
        m_stats.maxSubdivisionDepth = std::max(
            m_stats.maxSubdivisionDepth,
            static_cast<unsigned int>(cell.subdivisionDepth));
    }
}

void ProgressiveHexStrategy::BindCellToRecursivePath(HexCell& cell) const
{
    cell.path.clear();
    cell.subregionIndex = -1;
    cell.nestedSubregionIndex = -1;
    cell.subdivisionDepth = 0;
    cell.majorCenterPassIndex = GetMajorCenterPassIndex(cell);
    cell.childCenterPassIndex = -1;
    cell.nestedCenterPassIndex = -1;

    if (cell.regionIndex < 0 || static_cast<std::size_t>(cell.regionIndex) >= m_regionCenters.size())
    {
        return;
    }

    Hex parentCenter = m_regionCenters[static_cast<std::size_t>(cell.regionIndex)];
    int parentRadius = m_stats.majorCenterRegionRadius;
    int childRadius = ComputeChildRegionRadiusForPath(cell.regionIndex, cell.path, parentRadius);

    while (CanSubdivideRegion(parentRadius, childRadius))
    {
        const unsigned int maxDepth = ComputeMaxSubdivisionDepthForPath(cell.regionIndex, cell.path);
        if (maxDepth > 0U && cell.path.size() >= maxDepth)
        {
            break;
        }

        const std::vector<Hex> childCenters = BuildChildRegionCenters(parentCenter, parentRadius, childRadius);
        const int childIndex = SelectChildCenterIndexForCell(cell, parentCenter, parentRadius, childCenters);
        if (childIndex < 0)
        {
            break;
        }

        cell.path.push_back(childIndex);
        cell.subdivisionDepth = static_cast<int>(cell.path.size());
        if (cell.path.size() == 1U)
        {
            cell.subregionIndex = childIndex;
        }
        else if (cell.path.size() == 2U)
        {
            cell.nestedSubregionIndex = childIndex;
        }

        parentCenter = childCenters[static_cast<std::size_t>(childIndex)];
        parentRadius = childRadius;
        childRadius = ComputeChildRegionRadiusForPath(cell.regionIndex, cell.path, parentRadius);
    }
}

void ProgressiveHexStrategy::BuildCenterPassList()
{
    m_centerPasses.clear();

    for (std::size_t regionIndex = 0; regionIndex < m_regionCenters.size(); ++regionIndex)
    {
        const std::vector<int> rootPath;
        AppendCenterPass(
            'M',
            0U,
            static_cast<int>(regionIndex),
            rootPath,
            m_regionCenters[regionIndex],
            nullptr,
            nullptr);
        AppendRecursiveCenterPasses(
            static_cast<int>(regionIndex),
            rootPath,
            m_regionCenters[regionIndex],
            m_stats.majorCenterRegionRadius,
            0U);
    }
}

void ProgressiveHexStrategy::AppendRecursiveCenterPasses(
    int regionIndex,
    const std::vector<int>& parentPath,
    const Hex& parentCenter,
    int parentRadius,
    unsigned int parentDepth)
{
    const unsigned int maxDepth = ComputeMaxSubdivisionDepthForPath(regionIndex, parentPath);
    if (maxDepth > 0U && parentDepth >= maxDepth)
    {
        return;
    }

    const int childRadius = ComputeChildRegionRadiusForPath(regionIndex, parentPath, parentRadius);
    if (!CanSubdivideRegion(parentRadius, childRadius))
    {
        return;
    }

    const std::vector<Hex> childCenters = BuildChildRegionCenters(parentCenter, parentRadius, childRadius);
    for (std::size_t childIndex = 0; childIndex < childCenters.size(); ++childIndex)
    {
        std::vector<int> childPath = parentPath;
        childPath.push_back(static_cast<int>(childIndex));
        if (!HasCellsForPath(regionIndex, childPath))
        {
            continue;
        }

        const unsigned int childDepth = parentDepth + 1U;
        const char marker = childDepth == 1U ? 'C' : 'N';
        AppendCenterPass(
            marker,
            childDepth,
            regionIndex,
            childPath,
            childCenters[childIndex],
            &parentCenter,
            nullptr);
        AppendRecursiveCenterPasses(
            regionIndex,
            childPath,
            childCenters[childIndex],
            childRadius,
            childDepth);
    }
}

bool ProgressiveHexStrategy::HasCellsForPath(int regionIndex, const std::vector<int>& path) const
{
    for (const HexCell& cell : m_hexCells)
    {
        if (cell.regionIndex != regionIndex || cell.path.size() < path.size())
        {
            continue;
        }

        bool matches = true;
        for (std::size_t index = 0; index < path.size(); ++index)
        {
            if (cell.path[index] != path[index])
            {
                matches = false;
                break;
            }
        }

        if (matches)
        {
            return true;
        }
    }

    return false;
}

int ProgressiveHexStrategy::FindCenterPassIndexByPath(int regionIndex, const std::vector<int>& path) const
{
    for (std::size_t index = 0; index < m_centerPasses.size(); ++index)
    {
        const CenterPass& pass = m_centerPasses[index];
        if (pass.regionIndex == regionIndex && pass.path == path)
        {
            return static_cast<int>(index);
        }
    }

    return -1;
}

int ProgressiveHexStrategy::FindCenterPassIndex(
    int regionIndex,
    int subregionIndex,
    int nestedSubregionIndex) const
{
    for (std::size_t index = 0; index < m_centerPasses.size(); ++index)
    {
        const CenterPass& pass = m_centerPasses[index];
        if (pass.regionIndex == regionIndex &&
            pass.subregionIndex == subregionIndex &&
            pass.nestedSubregionIndex == nestedSubregionIndex)
        {
            return static_cast<int>(index);
        }
    }

    return -1;
}

void ProgressiveHexStrategy::AssignCenterPassIndicesToCells()
{
    for (HexCell& cell : m_hexCells)
    {
        cell.majorCenterPassIndex = FindCenterPassIndex(cell.regionIndex, -1, -1);
        cell.childCenterPassIndex = -1;
        cell.nestedCenterPassIndex = -1;
        if (!cell.path.empty())
        {
            std::vector<int> childPath;
            childPath.push_back(cell.path[0]);
            cell.childCenterPassIndex = FindCenterPassIndexByPath(cell.regionIndex, childPath);
        }
        if (cell.path.size() >= 2U)
        {
            std::vector<int> nestedPath;
            nestedPath.push_back(cell.path[0]);
            nestedPath.push_back(cell.path[1]);
            cell.nestedCenterPassIndex = FindCenterPassIndexByPath(cell.regionIndex, nestedPath);
        }
    }
}

void ProgressiveHexStrategy::BuildCenterPassConstituentLists()
{
    m_centerPassConstituentCellIndices.clear();
    m_centerPassConstituentCellIndices.resize(m_centerPasses.size());

    for (std::size_t passIndex = 0; passIndex < m_centerPasses.size(); ++passIndex)
    {
        const CenterPass& pass = m_centerPasses[passIndex];
        std::vector<std::size_t>& constituents = m_centerPassConstituentCellIndices[passIndex];
        for (std::size_t cellIndex = 0; cellIndex < m_hexCells.size(); ++cellIndex)
        {
            if (IsCellConstituentOfPass(m_hexCells[cellIndex], pass))
            {
                constituents.push_back(cellIndex);
            }
        }
    }
}

unsigned int ProgressiveHexStrategy::ComputeCenterPassRepeatCount(const CenterPass& pass) const
{
    if (pass.depth > m_stats.maxSubdivisionDepth)
    {
        return 1U;
    }

    const unsigned int depthLevel = pass.depth + 1U;
    unsigned int centerLineageMultiplier = 1U;
    if (pass.regionIndex == 0)
    {
        unsigned int leadingCenterDepth = 1U;
        for (int childIndex : pass.path)
        {
            if (childIndex != 0)
            {
                break;
            }

            ++leadingCenterDepth;
        }

        centerLineageMultiplier += leadingCenterDepth * 2U;
    }

    return depthLevel * depthLevel * centerLineageMultiplier;
}

void ProgressiveHexStrategy::AppendCenterPass(
    char marker,
    unsigned int depth,
    int regionIndex,
    int subregionIndex,
    int nestedSubregionIndex,
    const Hex& center,
    const Hex* parent,
    const Hex* grandparent)
{
    std::vector<int> path;
    if (subregionIndex >= 0)
    {
        path.push_back(subregionIndex);
    }
    if (nestedSubregionIndex >= 0)
    {
        path.push_back(nestedSubregionIndex);
    }

    AppendCenterPass(marker, depth, regionIndex, path, center, parent, grandparent);
}

void ProgressiveHexStrategy::AppendCenterPass(
    char marker,
    unsigned int depth,
    int regionIndex,
    const std::vector<int>& path,
    const Hex& center,
    const Hex* parent,
    const Hex* grandparent)
{
    CenterPass pass;
    pass.symbol = BuildSymbolPath(regionIndex, path);
    pass.marker = marker;
    pass.passIndex = static_cast<unsigned int>(m_centerPasses.size());
    pass.depth = depth;
    pass.regionIndex = regionIndex;
    pass.subregionIndex = path.empty() ? -1 : path[0];
    pass.nestedSubregionIndex = path.size() < 2U ? -1 : path[1];
    pass.path = path;
    pass.center = center;
    if (parent)
    {
        pass.parent = *parent;
        pass.hasParent = true;
    }
    if (grandparent)
    {
        pass.grandparent = *grandparent;
        pass.hasGrandparent = true;
    }
    m_centerPasses.push_back(pass);
}

void ProgressiveHexStrategy::AssignRecursiveSubdivision(HexCell& cell, const Hex& macroCenter) const
{
    Hex parentCenter = macroCenter;
    int parentRadius = m_stats.majorCenterRegionRadius;
    int childRadius = ComputeChildRegionRadius(parentRadius);
    int depth = 0;

    while (CanSubdivideRegion(parentRadius, childRadius))
    {
        const std::vector<Hex> childCenters = BuildChildRegionCenters(parentCenter, parentRadius, childRadius);
        const int nearestChildIndex = FindClosestChildCenterIndex(cell.coord, childCenters);

        if (nearestChildIndex < 0)
        {
            return;
        }

        cell.subregionIndex = nearestChildIndex;
        cell.nestedSubregionIndex = -1;
        cell.subdivisionDepth = depth + 1;
        const int nearestDistance = ComputeHexDistance(
            cell.coord,
            childCenters[static_cast<std::size_t>(nearestChildIndex)]);
        if (nearestDistance > childRadius)
        {
            return;
        }

        parentCenter = childCenters[static_cast<std::size_t>(nearestChildIndex)];
        parentRadius = childRadius;
        childRadius = ComputeChildRegionRadius(parentRadius);
        ++depth;
    }
}

void ProgressiveHexStrategy::BuildTraversalFillOrder()
{
    if (!m_centerPasses.empty())
    {
        struct WeightedCenterPass
        {
            unsigned int centerPassIndex = 0U;
            unsigned int repeatCount = 1U;
        };
        struct ScheduledCenterPass
        {
            unsigned int centerPassIndex = 0U;
            unsigned int repeatOrdinal = 0U;
            float phase = 0.0f;
        };

        std::vector<WeightedCenterPass> weightedPasses;
        weightedPasses.reserve(m_centerPasses.size());
        std::size_t totalSlotCount = 0U;
        for (std::size_t index = 0; index < m_centerPasses.size(); ++index)
        {
            const unsigned int centerPassIndex = static_cast<unsigned int>(index);
            const unsigned int repeatCount = ComputeCenterPassRepeatCount(m_centerPasses[index]);
            weightedPasses.push_back({ centerPassIndex, repeatCount });
            totalSlotCount += repeatCount;
        }

        std::vector<ScheduledCenterPass> scheduledPasses;
        scheduledPasses.reserve(totalSlotCount);
        for (const WeightedCenterPass& weightedPass : weightedPasses)
        {
            const float passOffset = std::fmod(
                static_cast<float>((weightedPass.centerPassIndex * 0.61803398875) + 0.2113248654),
                1.0f);
            for (unsigned int repeatIndex = 0U; repeatIndex < weightedPass.repeatCount; ++repeatIndex)
            {
                float phase =
                    (static_cast<float>(repeatIndex) + passOffset) /
                    static_cast<float>(weightedPass.repeatCount);
                phase -= std::floor(phase);
                scheduledPasses.push_back({ weightedPass.centerPassIndex, repeatIndex, phase });
            }
        }

        std::sort(
            scheduledPasses.begin(),
            scheduledPasses.end(),
            [this](const ScheduledCenterPass& leftPass, const ScheduledCenterPass& rightPass)
            {
                const unsigned int leftIndex = leftPass.centerPassIndex;
                const unsigned int rightIndex = rightPass.centerPassIndex;
                const CenterPass& left = m_centerPasses[leftIndex];
                const CenterPass& right = m_centerPasses[rightIndex];
                if (leftPass.phase != rightPass.phase)
                {
                    return leftPass.phase < rightPass.phase;
                }
                if (leftPass.repeatOrdinal != rightPass.repeatOrdinal)
                {
                    return leftPass.repeatOrdinal < rightPass.repeatOrdinal;
                }
                const int leftDistance = ComputeHexDistanceFromCenter(left.center);
                const int rightDistance = ComputeHexDistanceFromCenter(right.center);
                if (leftDistance != rightDistance)
                {
                    return leftDistance < rightDistance;
                }
                if (left.depth != right.depth)
                {
                    return left.depth < right.depth;
                }
                return left.passIndex < right.passIndex;
            });

        for (const ScheduledCenterPass& scheduledPass : scheduledPasses)
        {
            MarkCenterPassFilled(scheduledPass.centerPassIndex);
        }
        return;
    }

    std::vector<std::size_t> indices;
    indices.reserve(m_hexCells.size());
    for (std::size_t index = 0; index < m_hexCells.size(); ++index)
    {
        indices.push_back(index);
    }

    std::sort(
        indices.begin(),
        indices.end(),
        [this](std::size_t leftIndex, std::size_t rightIndex)
        {
            const HexCell& left = m_hexCells[leftIndex];
            const HexCell& right = m_hexCells[rightIndex];
            if (left.regionIndex != right.regionIndex)
            {
                return left.regionIndex < right.regionIndex;
            }
            if (left.subdivisionDepth != right.subdivisionDepth)
            {
                return left.subdivisionDepth < right.subdivisionDepth;
            }
            return ComputeHexDistanceFromCenter(left.coord) < ComputeHexDistanceFromCenter(right.coord);
        });

    for (std::size_t index : indices)
    {
        MarkCellFilled(index, false);
    }
}

std::vector<ProgressiveHexStrategy::Hex> ProgressiveHexStrategy::BuildTerminalRegionCells(
    const RegionWork& work) const
{
    std::vector<Hex> result;
    const std::vector<Hex> spiral = GenerateSpiral(work.center, work.radius);
    for (const Hex& hex : spiral)
    {
        const auto it = m_hexCellIndexByCoord.find(PackHexKey(hex));
        if (it != m_hexCellIndexByCoord.end())
        {
            result.push_back(hex);
        }
    }
    return result;
}

void ProgressiveHexStrategy::AppendTerminalLeafGroupsRoundRobin(const std::vector<std::vector<Hex>>& terminalGroups)
{
    std::size_t maxGroupSize = 0U;
    for (const std::vector<Hex>& group : terminalGroups)
    {
        maxGroupSize = std::max(maxGroupSize, group.size());
    }

    for (std::size_t index = 0; index < maxGroupSize; ++index)
    {
        for (const std::vector<Hex>& group : terminalGroups)
        {
            if (index < group.size())
            {
                TryMarkFillHex(group[index]);
            }
        }
    }
}

void ProgressiveHexStrategy::AppendUnfilledCoveragePass()
{
    for (std::size_t index = 0; index < m_hexCells.size(); ++index)
    {
        MarkCellFilled(index, false);
    }
}

void ProgressiveHexStrategy::TryMarkFillHex(const Hex& hex)
{
    const auto it = m_hexCellIndexByCoord.find(PackHexKey(hex));
    if (it != m_hexCellIndexByCoord.end())
    {
        MarkCellFilled(it->second, false);
    }
}

void ProgressiveHexStrategy::MarkCellFilled(std::size_t cellIndex, bool allowRepeat)
{
    if (cellIndex >= m_hexCells.size())
    {
        return;
    }

    HexCell& cell = m_hexCells[cellIndex];
    if (!allowRepeat && cell.fillOrderIndex >= 0)
    {
        return;
    }

    if (cell.fillOrderIndex < 0)
    {
        cell.fillOrderIndex = static_cast<int>(m_stats.fillStepCount);
    }

    m_fillOrderCellIndices.push_back(cellIndex);
    ++m_stats.fillStepCount;
}

void ProgressiveHexStrategy::MarkCenterPassFilled(unsigned int centerPassIndex)
{
    if (centerPassIndex >= m_centerPasses.size())
    {
        return;
    }

    m_fillOrderCenterPassIndices.push_back(centerPassIndex);
    ++m_stats.fillStepCount;
}

void ProgressiveHexStrategy::ProcessFillPassBatch(
    unsigned int passCount,
    unsigned int workerCount,
    const ProgressiveHexJobSubmitter& submitJob,
    const ProgressiveHexJobWaiter& waitForJobs)
{
    (void)submitJob;
    (void)waitForJobs;
    m_stats.lastParallelWorkerCount = std::max(1U, workerCount);
    m_stats.lastParallelJobCount = 0U;
    m_stats.lastParallelSampleCount = passCount;

    for (unsigned int index = 0U; index < passCount; ++index)
    {
        const std::size_t fillIndex =
            static_cast<std::size_t>(m_stats.visibleFillStepCount + index) % m_stats.fillStepCount;
        if (!m_fillOrderCenterPassIndices.empty())
        {
            const unsigned int centerPassIndex = m_fillOrderCenterPassIndices[fillIndex];
            ProcessCenterPass(m_centerPasses[centerPassIndex]);
        }
        else
        {
            HexCell& cell = m_hexCells[m_fillOrderCellIndices[fillIndex]];
            ApplyFillPassResult(cell, SampleCellColor(cell));
            cell.lastDepthMatchedPassTime = m_totalTime;
        }
    }

    const unsigned int advancedStepCount = m_stats.visibleFillStepCount + passCount;
    if (advancedStepCount >= m_stats.fillStepCount)
    {
        m_stats.visibleFillStepCount = advancedStepCount % m_stats.fillStepCount;
        m_stats.fillLoopCycleCount += advancedStepCount / m_stats.fillStepCount;
        if (!m_config.persistentPaint)
        {
            m_regionVisualDepths.assign(m_regionCenters.size(), -1);
            for (HexCell& cell : m_hexCells)
            {
                cell.lastFillPassTime = -1.0;
                cell.lastDepthMatchedPassTime = -1.0;
                cell.resolvedColor = 0xFF000000U;
                cell.lod1DirectionalColors.fill(0xFF000000U);
                cell.resolvedSampleCount = 0U;
                cell.hasResolvedColor = false;
                cell.hasLod1DirectionalDetail = false;
                cell.hasTerminalDetail = false;
                cell.hasDetailRaster = false;
                cell.detailRasterGridSize = 0U;
                cell.detailRasterValid.fill(false);
            }
        }
    }
    else
    {
        m_stats.visibleFillStepCount = advancedStepCount;
    }
}

void ProgressiveHexStrategy::ProcessCenterPass(const CenterPass& pass)
{
    const std::uint32_t color = SampleHexColor(pass.center);
    const float newSampleWeight = 0.20f / static_cast<float>(pass.depth + 1U);
    const bool terminalPass = pass.depth >= m_stats.maxSubdivisionDepth;

    if (pass.passIndex < m_centerPassConstituentCellIndices.size())
    {
        for (std::size_t cellIndex : m_centerPassConstituentCellIndices[pass.passIndex])
        {
            if (cellIndex >= m_hexCells.size())
            {
                continue;
            }

            ProcessCenterPassCell(m_hexCells[cellIndex], pass, color, newSampleWeight, terminalPass);
        }
        return;
    }

    for (HexCell& cell : m_hexCells)
    {
        if (IsCellConstituentOfPass(cell, pass))
        {
            ProcessCenterPassCell(cell, pass, color, newSampleWeight, terminalPass);
        }
    }
}

void ProgressiveHexStrategy::ProcessCenterPassCell(
    HexCell& cell,
    const CenterPass& pass,
    std::uint32_t inheritedColor,
    float newSampleWeight,
    bool terminalPass)
{
    const unsigned int cellLod = ComputeCellRenderLod(cell);
    const std::uint32_t resolvedColor =
        cellLod > 0U ? SampleCellColorWithLod(cell, cellLod) : inheritedColor;
    ApplyFillPassResult(cell, resolvedColor, newSampleWeight);

    if (cellLod == 1U)
    {
        RenderCellDirectionalDetail(cell);
        cell.hasDetailRaster = false;
        cell.detailRasterGridSize = 0U;
        cell.detailRasterValid.fill(false);
    }
    else if (cellLod >= 2U)
    {
        cell.hasLod1DirectionalDetail = false;
        RenderCellRasterDetail(cell, cellLod);
    }
    else
    {
        cell.hasLod1DirectionalDetail = false;
        cell.hasDetailRaster = false;
        cell.detailRasterGridSize = 0U;
        cell.detailRasterValid.fill(false);
    }

    if (pass.depth >= static_cast<unsigned int>(std::max(0, cell.subdivisionDepth)))
    {
        cell.lastDepthMatchedPassTime = m_totalTime;
    }

    if (terminalPass && static_cast<unsigned int>(cell.subdivisionDepth) >= m_stats.maxSubdivisionDepth)
    {
        RenderTerminalCellDetail(cell, newSampleWeight);
    }
}

void ProgressiveHexStrategy::RenderTerminalCellDetail(HexCell& cell, float newSampleWeight)
{
    (void)newSampleWeight;

    if (!m_config.renderTerminalHexDetail || !m_sampleProvider)
    {
        cell.hasTerminalDetail = true;
        return;
    }

    float centerX = 0.0f;
    float centerY = 0.0f;
    centerX = cell.centerX;
    centerY = cell.centerY;

    const float sampleRadiusX = (m_config.hexSize * 0.48f) / m_aspectRatio;
    const float sampleRadiusY = m_config.hexSize * 0.48f;
    for (std::size_t index = 0U; index < TerminalDetailSampleCount; ++index)
    {
        const float sampleX = centerX + (TerminalDetailOffsets[index][0] * sampleRadiusX);
        const float sampleY = centerY + (TerminalDetailOffsets[index][1] * sampleRadiusY);
        cell.terminalDetailColors[index] = m_sampleProvider(sampleX, sampleY, m_totalTime);
    }

    cell.hasTerminalDetail = true;
}

void ProgressiveHexStrategy::RenderCellDirectionalDetail(HexCell& cell)
{
    if (!m_sampleProvider)
    {
        cell.hasLod1DirectionalDetail = false;
        return;
    }

    const float centerX = cell.centerX;
    const float centerY = cell.centerY;

    const float sampleRadiusX = (m_config.hexSize * 0.42f) / m_aspectRatio;
    const float sampleRadiusY = m_config.hexSize * 0.18f;
    for (std::size_t index = 0U; index < 2U; ++index)
    {
        const float sampleX = centerX + (Lod1SampleOffsets[index][0] * sampleRadiusX);
        const float sampleY = centerY + (Lod1SampleOffsets[index][1] * sampleRadiusY);
        cell.lod1DirectionalColors[index] = m_sampleProvider(sampleX, sampleY, m_totalTime);
    }

    cell.hasLod1DirectionalDetail = true;
}

void ProgressiveHexStrategy::RenderCellRasterDetail(HexCell& cell, unsigned int lod)
{
    if (!m_sampleProvider || lod == 0U)
    {
        cell.hasDetailRaster = false;
        cell.detailRasterGridSize = 0U;
        cell.detailRasterValid.fill(false);
        return;
    }

    const unsigned int gridSize = ComputeCellRasterGridSize(lod);
    if (gridSize == 0U || gridSize > MaxCellRasterGridSize)
    {
        cell.hasDetailRaster = false;
        cell.detailRasterGridSize = 0U;
        cell.detailRasterValid.fill(false);
        return;
    }

    const float centerX = cell.centerX;
    const float centerY = cell.centerY;

    const float sampleRadiusX = (m_config.hexSize * 0.94f) / m_aspectRatio;
    const float sampleRadiusY = m_config.hexSize * 0.94f;
    const float invGridSize = 1.0f / static_cast<float>(gridSize);

    cell.detailRasterColors.fill(0xFF000000U);
    cell.detailRasterValid.fill(false);
    cell.detailRasterGridSize = gridSize;

    for (unsigned int row = 0U; row < gridSize; ++row)
    {
        for (unsigned int column = 0U; column < gridSize; ++column)
        {
            const std::size_t rasterIndex =
                static_cast<std::size_t>(row * MaxCellRasterGridSize) + column;
            const float normalizedX = (((static_cast<float>(column) + 0.5f) * invGridSize) * 2.0f) - 1.0f;
            const float normalizedY = (((static_cast<float>(row) + 0.5f) * invGridSize) * 2.0f) - 1.0f;
            if (!IsLocalPointInsideHex(normalizedX, normalizedY))
            {
                continue;
            }

            const float sampleX = centerX + (normalizedX * sampleRadiusX);
            const float sampleY = centerY + (normalizedY * sampleRadiusY);
            cell.detailRasterColors[rasterIndex] = m_sampleProvider(sampleX, sampleY, m_totalTime);
            cell.detailRasterValid[rasterIndex] = true;
        }
    }

    cell.hasDetailRaster = true;
}

bool ProgressiveHexStrategy::IsCellConstituentOfPass(const HexCell& cell, const CenterPass& pass) const
{
    if (cell.regionIndex != pass.regionIndex)
    {
        return false;
    }

    if (pass.path.empty())
    {
        return true;
    }

    if (cell.path.size() < pass.path.size())
    {
        return false;
    }

    for (std::size_t index = 0; index < pass.path.size(); ++index)
    {
        if (cell.path[index] != pass.path[index])
        {
            return false;
        }
    }

    return true;
}

void ProgressiveHexStrategy::ApplyFillPassResult(HexCell& cell, std::uint32_t color)
{
    ApplyFillPassResult(cell, color, 0.35f);
}

void ProgressiveHexStrategy::ApplyFillPassResult(HexCell& cell, std::uint32_t color, float newSampleWeight)
{
    if (cell.hasResolvedColor)
    {
        cell.resolvedColor = BlendColors(cell.resolvedColor, color, newSampleWeight);
    }
    else
    {
        cell.resolvedColor = color;
    }

    ++cell.resolvedSampleCount;
    cell.hasResolvedColor = true;
    cell.lastFillPassTime = m_totalTime;

    const int centerDepth = ComputeHierarchyCenterDepth(cell);
    if (centerDepth >= 0)
    {
        ApplyRegionVisualDepth(cell.regionIndex, centerDepth + 1);
    }
}

std::uint32_t ProgressiveHexStrategy::BlendColors(
    std::uint32_t previousColor,
    std::uint32_t nextColor,
    float nextWeight)
{
    const float clampedNextWeight = std::max(0.0f, std::min(1.0f, nextWeight));
    const float previousWeight = 1.0f - clampedNextWeight;

    const auto blendChannel =
        [previousColor, nextColor, previousWeight, clampedNextWeight](unsigned int shift) -> std::uint32_t
        {
            const float previous = static_cast<float>((previousColor >> shift) & 0xFFU);
            const float next = static_cast<float>((nextColor >> shift) & 0xFFU);
            return static_cast<std::uint32_t>((previous * previousWeight) + (next * clampedNextWeight) + 0.5f);
        };

    return
        0xFF000000U |
        (blendChannel(16U) << 16U) |
        (blendChannel(8U) << 8U) |
        blendChannel(0U);
}

int ProgressiveHexStrategy::ComputeHierarchyCenterDepth(const HexCell& cell) const
{
    if (cell.regionIndex < 0 || static_cast<std::size_t>(cell.regionIndex) >= m_regionCenters.size())
    {
        return -1;
    }

    const Hex& regionCenter = m_regionCenters[static_cast<std::size_t>(cell.regionIndex)];
    if (cell.coord.q == regionCenter.q && cell.coord.r == regionCenter.r)
    {
        return 0;
    }

    return -1;
}

void ProgressiveHexStrategy::ApplyRegionVisualDepth(int regionIndex, int visualDepth)
{
    if (regionIndex < 0 || static_cast<std::size_t>(regionIndex) >= m_regionVisualDepths.size())
    {
        return;
    }

    m_regionVisualDepths[static_cast<std::size_t>(regionIndex)] = std::max(
        m_regionVisualDepths[static_cast<std::size_t>(regionIndex)],
        visualDepth);
}

bool ProgressiveHexStrategy::IsMajorRegionCenter(const HexCell& cell) const
{
    if (cell.regionIndex < 0 || static_cast<std::size_t>(cell.regionIndex) >= m_regionCenters.size())
    {
        return false;
    }

    const Hex& regionCenter = m_regionCenters[static_cast<std::size_t>(cell.regionIndex)];
    return cell.coord.q == regionCenter.q && cell.coord.r == regionCenter.r;
}

bool ProgressiveHexStrategy::IsMajorRegionChildCenter(const HexCell& cell) const
{
    if (m_regionCenters.empty() ||
        cell.regionIndex < 0 ||
        static_cast<std::size_t>(cell.regionIndex) >= m_regionCenters.size())
    {
        return false;
    }

    const std::vector<Hex> childCenters = BuildMajorRegionChildCenters(cell.regionIndex);

    for (const Hex& childCenter : childCenters)
    {
        if (cell.coord.q == childCenter.q && cell.coord.r == childCenter.r)
        {
            return true;
        }
    }

    return false;
}

bool ProgressiveHexStrategy::IsNestedSubregionChildCenter(const HexCell& cell) const
{
    if (m_regionCenters.empty() ||
        cell.regionIndex < 0 ||
        static_cast<std::size_t>(cell.regionIndex) >= m_regionCenters.size())
    {
        return false;
    }

    const std::vector<Hex> childCenters = BuildMajorRegionChildCenters(cell.regionIndex);
    for (std::size_t childIndex = 0; childIndex < childCenters.size(); ++childIndex)
    {
        const std::vector<Hex> nestedChildCenters = BuildSubregionChildCenters(
            cell.regionIndex,
            static_cast<int>(childIndex));
        for (const Hex& nestedChildCenter : nestedChildCenters)
        {
            if (cell.coord.q == nestedChildCenter.q && cell.coord.r == nestedChildCenter.r)
            {
                return true;
            }
        }
    }

    return false;
}

unsigned int ProgressiveHexStrategy::CountAlgorithmCenterPasses() const
{
    unsigned int passCount = static_cast<unsigned int>(m_regionCenters.size());
    for (std::size_t regionIndex = 0; regionIndex < m_regionCenters.size(); ++regionIndex)
    {
        const std::vector<Hex> childCenters = BuildMajorRegionChildCenters(static_cast<int>(regionIndex));
        passCount += static_cast<unsigned int>(childCenters.size());
        for (std::size_t childIndex = 0; childIndex < childCenters.size(); ++childIndex)
        {
            passCount += static_cast<unsigned int>(
                BuildSubregionChildCenters(static_cast<int>(regionIndex), static_cast<int>(childIndex)).size());
        }
    }

    return passCount;
}

int ProgressiveHexStrategy::GetMajorCenterPassIndex(const HexCell& cell) const
{
    if (cell.majorCenterPassIndex >= 0)
    {
        return cell.majorCenterPassIndex;
    }

    if (cell.regionIndex < 0 || static_cast<std::size_t>(cell.regionIndex) >= m_regionCenters.size())
    {
        return -1;
    }

    return cell.regionIndex;
}

int ProgressiveHexStrategy::GetChildCenterPassIndex(const HexCell& cell) const
{
    if (cell.childCenterPassIndex >= 0)
    {
        return cell.childCenterPassIndex;
    }

    if (cell.regionIndex < 0 ||
        static_cast<std::size_t>(cell.regionIndex) >= m_regionCenters.size() ||
        cell.subregionIndex < 0)
    {
        return -1;
    }

    unsigned int passIndex = static_cast<unsigned int>(m_regionCenters.size());
    for (int regionIndex = 0; regionIndex < cell.regionIndex; ++regionIndex)
    {
        passIndex += static_cast<unsigned int>(BuildMajorRegionChildCenters(regionIndex).size());
    }

    return static_cast<int>(passIndex + static_cast<unsigned int>(cell.subregionIndex));
}

int ProgressiveHexStrategy::GetNestedCenterPassIndex(const HexCell& cell) const
{
    if (cell.nestedCenterPassIndex >= 0)
    {
        return cell.nestedCenterPassIndex;
    }

    if (cell.regionIndex < 0 ||
        static_cast<std::size_t>(cell.regionIndex) >= m_regionCenters.size() ||
        cell.subregionIndex < 0 ||
        cell.nestedSubregionIndex < 0)
    {
        return -1;
    }

    unsigned int passIndex = static_cast<unsigned int>(m_regionCenters.size());
    for (std::size_t regionIndex = 0; regionIndex < m_regionCenters.size(); ++regionIndex)
    {
        passIndex += static_cast<unsigned int>(BuildMajorRegionChildCenters(static_cast<int>(regionIndex)).size());
    }

    for (int regionIndex = 0; regionIndex < cell.regionIndex; ++regionIndex)
    {
        const std::vector<Hex> childCenters = BuildMajorRegionChildCenters(regionIndex);
        for (std::size_t childIndex = 0; childIndex < childCenters.size(); ++childIndex)
        {
            passIndex += static_cast<unsigned int>(
                BuildSubregionChildCenters(regionIndex, static_cast<int>(childIndex)).size());
        }
    }

    for (int childIndex = 0; childIndex < cell.subregionIndex; ++childIndex)
    {
        passIndex += static_cast<unsigned int>(
            BuildSubregionChildCenters(cell.regionIndex, childIndex).size());
    }

    return static_cast<int>(passIndex + static_cast<unsigned int>(cell.nestedSubregionIndex));
}

bool ProgressiveHexStrategy::HasCenterPassRun(int passIndex) const
{
    return passIndex >= 0 && static_cast<unsigned int>(passIndex) < m_algorithmPreviewProgressCount;
}

std::uint32_t ProgressiveHexStrategy::BuildPreviewColor(int colorIndex, float scale) const
{
    return BuildPreviewColorForCycle(m_algorithmPreviewCycle, colorIndex, scale);
}

std::uint32_t ProgressiveHexStrategy::BuildPreviewColorForCycle(
    unsigned int cycle,
    int colorIndex,
    float scale) const
{
    const int shiftedColorIndex = colorIndex + static_cast<int>(cycle * 3U);
    return ScaleRgbPackedColor(GetMajorRegionBaseColor(shiftedColorIndex), scale);
}

std::uint32_t ProgressiveHexStrategy::BuildCompletedCycleColor(const HexCell& cell, unsigned int cycle) const
{
    if (cell.nestedSubregionIndex >= 0)
    {
        const int nestedColorIndex = (cell.subregionIndex * 5) + cell.nestedSubregionIndex;
        return BuildPreviewColorForCycle(cycle, nestedColorIndex, 0.86f);
    }

    if (cell.subregionIndex >= 0)
    {
        return BuildPreviewColorForCycle(cycle, cell.subregionIndex, 0.78f);
    }

    if (cell.regionIndex >= 0)
    {
        return BuildPreviewColorForCycle(cycle, cell.regionIndex, 0.52f);
    }

    return 0xFF27313AU;
}

std::uint32_t ProgressiveHexStrategy::BuildMajorRegionChildOwnershipColor(const HexCell& cell) const
{
    const int majorPassIndex = GetMajorCenterPassIndex(cell);
    if (!HasCenterPassRun(majorPassIndex))
    {
        return m_algorithmPreviewCycle > 0U
            ? BuildCompletedCycleColor(cell, m_algorithmPreviewCycle - 1U)
            : 0xFF27313AU;
    }

    if (IsMajorRegionCenter(cell))
    {
        return 0xFFFFF2A6U;
    }

    std::uint32_t valueColor = BuildPreviewColor(cell.regionIndex, 0.52f);
    const int childPassIndex = GetChildCenterPassIndex(cell);
    if (!HasCenterPassRun(childPassIndex))
    {
        return valueColor;
    }

    if (IsMajorRegionChildCenter(cell))
    {
        return 0xFFB7F7FFU;
    }

    if (cell.subregionIndex < 0)
    {
        return valueColor;
    }

    valueColor = BuildPreviewColor(cell.subregionIndex, 0.78f);
    const int nestedPassIndex = GetNestedCenterPassIndex(cell);
    if (!HasCenterPassRun(nestedPassIndex))
    {
        return valueColor;
    }

    if (IsNestedSubregionChildCenter(cell))
    {
        return 0xFFC8FF8AU;
    }

    const int nestedColorIndex = (cell.subregionIndex * 5) + cell.nestedSubregionIndex;
    return BuildPreviewColor(nestedColorIndex, 0.86f);
}

bool ProgressiveHexStrategy::IsHexFullyInsideScreen(const Hex& hex) const
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    GetHexCenter(hex, centerX, centerY);

    for (int corner = 0; corner < 6; ++corner)
    {
        const float angle = Pi / 3.0f * static_cast<float>(corner);
        const float cornerX = centerX + ((std::cos(angle) * m_config.hexSize) / m_aspectRatio);
        const float cornerY = centerY + (std::sin(angle) * m_config.hexSize);

        if (cornerX < -1.0f + ScreenEdgeEpsilon ||
            cornerX > 1.0f - ScreenEdgeEpsilon ||
            cornerY < -1.0f + ScreenEdgeEpsilon ||
            cornerY > 1.0f - ScreenEdgeEpsilon)
        {
            return false;
        }
    }

    return true;
}

void ProgressiveHexStrategy::GetHexCenter(const Hex& hex, float& outX, float& outY) const
{
    outX = (m_config.hexSize * 1.5f * static_cast<float>(hex.q)) / m_aspectRatio;
    outY = m_config.hexSize * Sqrt3 * (static_cast<float>(hex.r) + (static_cast<float>(hex.q) * 0.5f));
}

void ProgressiveHexStrategy::BuildCellPatch(
    std::vector<ProgressiveHexPatch>& outPatches,
    const HexCell& cell) const
{
    if (!cell.hasResolvedColor && !m_config.showUnfilledGrid)
    {
        return;
    }

    float fillScale = cell.hasResolvedColor ? 1.0f : 0.72f;
    int regionVisualDepth = -1;
    if (cell.regionIndex >= 0 && static_cast<std::size_t>(cell.regionIndex) < m_regionVisualDepths.size())
    {
        regionVisualDepth = m_regionVisualDepths[static_cast<std::size_t>(cell.regionIndex)];
    }

    if (regionVisualDepth >= 0)
    {
        const float depthScale = std::max(0.32f, 1.0f - (static_cast<float>(regionVisualDepth) * 0.14f));
        fillScale = std::max(fillScale, depthScale);
    }

    if (m_config.fadeByTimeSincePass && cell.hasResolvedColor)
    {
        const double passTime = cell.lastDepthMatchedPassTime >= 0.0
            ? cell.lastDepthMatchedPassTime
            : cell.lastFillPassTime;
        const double ageSeconds = passTime >= 0.0
            ? std::max(0.0, m_totalTime - passTime)
            : static_cast<double>(m_config.passFadeSeconds);
        const float ageRatio = static_cast<float>(std::min(
            1.0,
            ageSeconds / static_cast<double>(m_config.passFadeSeconds)));
        const float freshnessScale =
            1.0f - ((1.0f - m_config.passFadeFloor) * ageRatio);
        fillScale *= freshnessScale;
    }

    const std::uint32_t baseColor = m_config.useRegionOwnershipColors
        ? BuildRegionOwnershipColor(cell)
        : cell.resolvedColor;

    if (cell.hasLod1DirectionalDetail && !m_config.useRegionOwnershipColors)
    {
        const float centerX = cell.centerX;
        const float centerY = cell.centerY;

        ProgressiveHexPatch backgroundPatch;
        backgroundPatch.centerX = centerX;
        backgroundPatch.centerY = centerY;
        backgroundPatch.halfWidth = (m_config.hexSize * m_config.patchSplatScale) / m_aspectRatio;
        backgroundPatch.halfHeight = m_config.hexSize * m_config.patchSplatScale;
        backgroundPatch.rotationRadians = 0.0f;
        backgroundPatch.color = ScaleRgbPackedColor(baseColor, fillScale * 0.38f);
        backgroundPatch.useHexShape = true;
        outPatches.push_back(backgroundPatch);

        const float bandOffsetX = (m_config.hexSize * 0.24f) / m_aspectRatio;
        const float bandHalfWidth = (m_config.hexSize * 0.24f) / m_aspectRatio;
        const float bandHalfHeight = m_config.hexSize * 0.48f;
        for (std::size_t index = 0U; index < 2U; ++index)
        {
            ProgressiveHexPatch bandPatch;
            bandPatch.centerX = centerX + ((index == 0U ? -1.0f : 1.0f) * bandOffsetX);
            bandPatch.centerY = centerY;
            bandPatch.halfWidth = bandHalfWidth;
            bandPatch.halfHeight = bandHalfHeight;
            bandPatch.rotationRadians = 0.0f;
            bandPatch.color = ScaleRgbPackedColor(cell.lod1DirectionalColors[index], fillScale);
            bandPatch.useHexShape = false;
            outPatches.push_back(bandPatch);
        }
        return;
    }

    if (cell.hasDetailRaster &&
        !m_config.useRegionOwnershipColors &&
        cell.detailRasterGridSize > 0U)
    {
        const float centerX = cell.centerX;
        const float centerY = cell.centerY;

        const float rasterRadiusX = (m_config.hexSize * 0.94f) / m_aspectRatio;
        const float rasterRadiusY = m_config.hexSize * 0.94f;
        const float cellHalfWidth = (rasterRadiusX / static_cast<float>(cell.detailRasterGridSize)) * 0.72f;
        const float cellHalfHeight = (rasterRadiusY / static_cast<float>(cell.detailRasterGridSize)) * 0.72f;
        const float invGridSize = 1.0f / static_cast<float>(cell.detailRasterGridSize);

        ProgressiveHexPatch backgroundPatch;
        backgroundPatch.centerX = centerX;
        backgroundPatch.centerY = centerY;
        backgroundPatch.halfWidth = (m_config.hexSize * m_config.patchSplatScale) / m_aspectRatio;
        backgroundPatch.halfHeight = m_config.hexSize * m_config.patchSplatScale;
        backgroundPatch.rotationRadians = 0.0f;
        backgroundPatch.color = ScaleRgbPackedColor(baseColor, fillScale * 0.35f);
        backgroundPatch.useHexShape = true;
        outPatches.push_back(backgroundPatch);

        for (unsigned int row = 0U; row < cell.detailRasterGridSize; ++row)
        {
            for (unsigned int column = 0U; column < cell.detailRasterGridSize; ++column)
            {
                const std::size_t rasterIndex =
                    static_cast<std::size_t>(row * MaxCellRasterGridSize) + column;
                if (!cell.detailRasterValid[rasterIndex])
                {
                    continue;
                }

                const float normalizedX = (((static_cast<float>(column) + 0.5f) * invGridSize) * 2.0f) - 1.0f;
                const float normalizedY = (((static_cast<float>(row) + 0.5f) * invGridSize) * 2.0f) - 1.0f;

                ProgressiveHexPatch rasterPatch;
                rasterPatch.centerX = centerX + (normalizedX * rasterRadiusX);
                rasterPatch.centerY = centerY + (normalizedY * rasterRadiusY);
                rasterPatch.halfWidth = cellHalfWidth;
                rasterPatch.halfHeight = cellHalfHeight;
                rasterPatch.rotationRadians = 0.0f;
                rasterPatch.color = ScaleRgbPackedColor(cell.detailRasterColors[rasterIndex], fillScale);
                rasterPatch.useHexShape = false;
                outPatches.push_back(rasterPatch);
            }
        }
        return;
    }

    if (m_config.renderTerminalHexDetail &&
        cell.hasTerminalDetail &&
        !m_config.useRegionOwnershipColors &&
        static_cast<unsigned int>(cell.subdivisionDepth) >= m_stats.maxSubdivisionDepth)
    {
        const float centerX = cell.centerX;
        const float centerY = cell.centerY;

        const float sampleRadiusX = (m_config.hexSize * 0.48f) / m_aspectRatio;
        const float sampleRadiusY = m_config.hexSize * 0.48f;
        const float patchHalfWidth = (m_config.hexSize * 0.62f) / m_aspectRatio;
        const float patchHalfHeight = m_config.hexSize * 0.62f;

        for (std::size_t index = 0U; index < TerminalDetailSampleCount; ++index)
        {
            ProgressiveHexPatch patch;
            patch.centerX = centerX + (TerminalDetailOffsets[index][0] * sampleRadiusX);
            patch.centerY = centerY + (TerminalDetailOffsets[index][1] * sampleRadiusY);
            patch.halfWidth = patchHalfWidth;
            patch.halfHeight = patchHalfHeight;
            patch.rotationRadians = 0.0f;
            patch.color = cell.terminalDetailColors[index];
            patch.useHexShape = false;
            outPatches.push_back(patch);
        }
        return;
    }

    ProgressiveHexPatch patch;
    patch.centerX = cell.centerX;
    patch.centerY = cell.centerY;
    patch.halfWidth = (m_config.hexSize * m_config.patchSplatScale) / m_aspectRatio;
    patch.halfHeight = m_config.hexSize * m_config.patchSplatScale;
    patch.rotationRadians = 0.0f;
    patch.color = ScaleRgbPackedColor(baseColor, fillScale);
    patch.useHexShape = true;
    outPatches.push_back(patch);
}

std::uint32_t ProgressiveHexStrategy::BuildRegionOwnershipColor(const HexCell& cell) const
{
    if (m_config.highlightMajorRegionCenters)
    {
        if (m_algorithmPreviewStep == 0U)
        {
            return 0xFF27313AU;
        }

        if (m_algorithmPreviewStep == 1U)
        {
            return BuildMajorRegionChildOwnershipColor(cell);
        }

        if (cell.regionIndex < 0 ||
            static_cast<unsigned int>(cell.regionIndex) >= m_algorithmPreviewProgressCount)
        {
            return m_algorithmPreviewCycle > 0U
                ? BuildCompletedCycleColor(cell, m_algorithmPreviewCycle - 1U)
                : 0xFF27313AU;
        }

        return IsMajorRegionCenter(cell)
            ? BuildPreviewColor(cell.regionIndex, 1.22f)
            : BuildPreviewColor(cell.regionIndex, 1.0f);
    }

    if (cell.regionIndex < 0)
    {
        return 0xFF343A40U;
    }

    const float depthBand = std::max(0.42f, 1.0f - (static_cast<float>(cell.subdivisionDepth) * 0.16f));
    return BuildPreviewColor(cell.regionIndex, depthBand);
}

std::uint32_t ProgressiveHexStrategy::SampleHexColor(const Hex& hex) const
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    GetHexCenter(hex, centerX, centerY);

    if (m_sampleProvider)
    {
        return m_sampleProvider(centerX, centerY, m_totalTime);
    }

    return SampleDefaultGradient(centerX);
}

unsigned int ProgressiveHexStrategy::ComputePassRenderLod(const CenterPass& pass) const
{
    return ComputeRenderLod(pass.regionIndex, pass.path, pass.center);
}

unsigned int ProgressiveHexStrategy::ComputeCellRenderLod(const HexCell& cell) const
{
    return cell.renderLod;
}

unsigned int ProgressiveHexStrategy::ComputeRenderLod(
    int regionIndex,
    const std::vector<int>& path,
    const Hex& coord) const
{
    if (regionIndex != 0)
    {
        return 0U;
    }

    if (!path.empty() && path[0] != 0)
    {
        const int lod1Radius = std::max(4, (m_stats.majorCenterRegionRadius * 2) / 3);
        return ComputeHexDistanceFromCenter(coord) <= lod1Radius ? 1U : 0U;
    }

    unsigned int leadingCenterDepth = 0U;
    for (int childIndex : path)
    {
        if (childIndex != 0)
        {
            break;
        }

        ++leadingCenterDepth;
    }

    return leadingCenterDepth == 0U ? 0U : std::min(4U, leadingCenterDepth + 1U);
}

unsigned int ProgressiveHexStrategy::ComputeCellRasterGridSize(unsigned int lod)
{
    switch (lod)
    {
    case 2U:
        return 3U;
    case 3U:
        return 5U;
    default:
        return 9U;
    }
}

bool ProgressiveHexStrategy::IsLocalPointInsideHex(float normalizedX, float normalizedY)
{
    static constexpr float hexCorners[6][2] =
    {
        {  1.0f,  0.0f },
        {  0.5f,  TerminalDetailOffsetScale },
        { -0.5f,  TerminalDetailOffsetScale },
        { -1.0f,  0.0f },
        { -0.5f, -TerminalDetailOffsetScale },
        {  0.5f, -TerminalDetailOffsetScale }
    };

    bool inside = false;
    for (int current = 0, previous = 5; current < 6; previous = current++)
    {
        const float currentX = hexCorners[current][0];
        const float currentY = hexCorners[current][1];
        const float previousX = hexCorners[previous][0];
        const float previousY = hexCorners[previous][1];
        const bool crossesHorizontal = ((currentY > normalizedY) != (previousY > normalizedY));
        if (!crossesHorizontal)
        {
            continue;
        }

        const float edgeX =
            previousX + ((normalizedY - previousY) * (currentX - previousX) / (currentY - previousY));
        if (normalizedX < edgeX)
        {
            inside = !inside;
        }
    }

    return inside;
}

std::uint32_t ProgressiveHexStrategy::SampleCellColorWithLod(const HexCell& cell, unsigned int lod) const
{
    if (!m_sampleProvider || lod == 0U)
    {
        return SampleHexColor(cell.coord);
    }

    const float centerX = cell.centerX;
    const float centerY = cell.centerY;

    const float sampleRadiusX = (m_config.hexSize * 0.46f) / m_aspectRatio;
    const float sampleRadiusY = m_config.hexSize * 0.46f;

    const float (*sampleOffsets)[2] = Lod1SampleOffsets;
    std::size_t sampleCount = Lod1SampleCount;
    switch (lod)
    {
    case 1U:
        sampleOffsets = Lod1SampleOffsets;
        sampleCount = Lod1SampleCount;
        break;
    case 2U:
        sampleOffsets = Lod2SampleOffsets;
        sampleCount = Lod2SampleCount;
        break;
    case 3U:
        sampleOffsets = Lod3SampleOffsets;
        sampleCount = Lod3SampleCount;
        break;
    default:
        sampleOffsets = Lod4SampleOffsets;
        sampleCount = Lod4SampleCount;
        break;
    }

    float accumulatedRed = 0.0f;
    float accumulatedGreen = 0.0f;
    float accumulatedBlue = 0.0f;
    for (std::size_t index = 0U; index < sampleCount; ++index)
    {
        const float sampleX = centerX + (sampleOffsets[index][0] * sampleRadiusX);
        const float sampleY = centerY + (sampleOffsets[index][1] * sampleRadiusY);
        const std::uint32_t color = m_sampleProvider(sampleX, sampleY, m_totalTime);
        accumulatedRed += static_cast<float>((color >> 16U) & 0xFFU);
        accumulatedGreen += static_cast<float>((color >> 8U) & 0xFFU);
        accumulatedBlue += static_cast<float>(color & 0xFFU);
    }

    const float invSampleCount = 1.0f / static_cast<float>(sampleCount * 255U);
    return PackRgbColor(
        accumulatedRed * invSampleCount,
        accumulatedGreen * invSampleCount,
        accumulatedBlue * invSampleCount);
}

std::uint32_t ProgressiveHexStrategy::SampleCellColor(const HexCell& cell) const
{
    return SampleCellColorWithLod(cell, ComputeCellRenderLod(cell));
}

std::uint32_t ProgressiveHexStrategy::SampleDefaultGradient(float screenX) const
{
    const float t = Clamp01((screenX + 1.0f) * 0.5f);
    const std::uint32_t red = static_cast<std::uint32_t>((t * 255.0f) + 0.5f);
    const std::uint32_t blue = static_cast<std::uint32_t>(((1.0f - t) * 255.0f) + 0.5f);
    const std::uint32_t green = static_cast<std::uint32_t>((0.10f * 255.0f) + 0.5f);

    return 0xFF000000U | (red << 16U) | (green << 8U) | blue;
}
}
