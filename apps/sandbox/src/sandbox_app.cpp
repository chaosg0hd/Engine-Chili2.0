#include "sandbox_app.hpp"

#include "core/engine_core.hpp"
#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>

namespace
{
    constexpr float kCameraMoveSpeed = 7.0f;
    constexpr float kLightMoveSpeed = 4.0f;
    constexpr unsigned int kMinRayCount = 1U;
    constexpr unsigned int kMaxRayCount = 256U;
    constexpr double kRandomRayRefreshSeconds = 1.0;
    constexpr unsigned char kRestartKey = 'R';
    constexpr unsigned char kToggleDebugRaysKey = 'V';
    constexpr unsigned char kToggleHexObservationKey = 'H';
    constexpr unsigned char kDecreaseRaysKey = '[';
    constexpr unsigned char kIncreaseRaysKey = ']';
    constexpr double kHexObservationStepSeconds = 0.0;
    constexpr double kHexObservationRecursionUnlockSeconds = 0.25;
    constexpr double kHexObservationMainThreadBudgetSeconds = 0.002;
    constexpr unsigned int kHexObservationMaxStepsPerFrame = 128U;
    constexpr unsigned int kHexObservationMaxSamplesPerBatch = 256U;
    constexpr float kHexObservationSettleDifficulty = 0.055f;
    constexpr float kHexObservationStartRadius = 0.36f;
    constexpr float kHexObservationChildRadiusScale = 0.38f;

    struct HexObservationGpuSampleInput
    {
        float ndcX = 0.0f;
        float ndcY = 0.0f;
        std::uint32_t depth = 0U;
        std::uint32_t cellId = 0U;
        float forwardX = 0.0f;
        float forwardY = 0.0f;
        float forwardZ = 1.0f;
        float rightX = 1.0f;
        float rightY = 0.0f;
        float rightZ = 0.0f;
        float upX = 0.0f;
        float upY = 1.0f;
        float upZ = 0.0f;
        float aspect = 1.0f;
        float halfHeight = 1.0f;
        float originX = 0.0f;
        float originY = 0.0f;
        float originZ = 0.0f;
        float pad = 0.0f;
    };

    struct HexObservationGpuSampleOutput
    {
        std::uint32_t cellId = 0U;
        float rayX = 0.0f;
        float rayY = 0.0f;
        float rayZ = 1.0f;
        std::uint32_t sampleColor = 0xFF202020u;
        std::uint32_t pad0 = 0U;
        std::uint32_t pad1 = 0U;
        std::uint32_t pad2 = 0U;
    };

    static_assert((sizeof(HexObservationGpuSampleInput) % 4U) == 0U);
    static_assert((sizeof(HexObservationGpuSampleOutput) % 4U) == 0U);

    constexpr const char* kHexObservationGpuSampleShader = R"(
ByteAddressBuffer SampleInput : register(t0);
RWByteAddressBuffer SampleOutput : register(u0);

float LoadFloat(uint address)
{
    return asfloat(SampleInput.Load(address));
}

uint PackColor(float3 color)
{
    color = saturate(color);
    uint3 bytes = (uint3)(color * 255.0f);
    return 0xFF000000u | (bytes.r << 16) | (bytes.g << 8) | bytes.b;
}

float3 ShadeHit(uint baseColor, float3 normal, float3 rayDirection, float distance)
{
    float3 color = float3(
        (float)((baseColor >> 16) & 0xFFu) / 255.0f,
        (float)((baseColor >> 8) & 0xFFu) / 255.0f,
        (float)(baseColor & 0xFFu) / 255.0f);
    float3 lightDirection = normalize(float3(-0.35f, 0.8f, -0.4f));
    float diffuse = max(0.0f, dot(normalize(normal), lightDirection));
    float facing = 0.35f + (max(0.0f, dot(normalize(normal), -rayDirection)) * 0.25f);
    float distanceFade = 1.0f / (1.0f + (distance * 0.035f));
    float shade = (0.28f + (diffuse * 0.52f) + facing) * distanceFade;
    return color * shade;
}

bool TracePlane(float3 origin, float3 direction, float3 point, float3 normal, uint color, inout float nearest, inout uint outColor)
{
    float3 n = normalize(normal);
    float denominator = dot(direction, n);
    if (abs(denominator) <= 0.000001f)
    {
        return false;
    }

    float distance = dot(point - origin, n) / denominator;
    if (distance < 0.0f || distance >= nearest)
    {
        return false;
    }

    nearest = distance;
    outColor = PackColor(ShadeHit(color, denominator > 0.0f ? -n : n, direction, distance));
    return true;
}

bool TraceBox(float3 origin, float3 direction, float3 center, float3 halfExtents, uint color, inout float nearest, inout uint outColor)
{
    float3 minCorner = center - halfExtents;
    float3 maxCorner = center + halfExtents;
    float tMin = 0.0f;
    float tMax = nearest;
    float3 hitNormal = float3(0.0f, 1.0f, 0.0f);

    [unroll]
    for (uint axis = 0; axis < 3; ++axis)
    {
        float originAxis = origin[axis];
        float directionAxis = direction[axis];
        float minAxis = minCorner[axis];
        float maxAxis = maxCorner[axis];
        float3 axisNormal = axis == 0 ? float3(1.0f, 0.0f, 0.0f) : (axis == 1 ? float3(0.0f, 1.0f, 0.0f) : float3(0.0f, 0.0f, 1.0f));

        if (abs(directionAxis) <= 0.000001f)
        {
            if (originAxis < minAxis || originAxis > maxAxis)
            {
                return false;
            }
            continue;
        }

        float nearDistance = (minAxis - originAxis) / directionAxis;
        float farDistance = (maxAxis - originAxis) / directionAxis;
        float3 nearNormal = -axisNormal;
        if (nearDistance > farDistance)
        {
            float swapValue = nearDistance;
            nearDistance = farDistance;
            farDistance = swapValue;
            nearNormal = axisNormal;
        }

        if (nearDistance > tMin)
        {
            tMin = nearDistance;
            hitNormal = nearNormal;
        }

        tMax = min(tMax, farDistance);
        if (tMin > tMax)
        {
            return false;
        }
    }

    float hitDistance = tMin >= 0.0f ? tMin : tMax;
    if (hitDistance < 0.0f || hitDistance >= nearest)
    {
        return false;
    }

    nearest = hitDistance;
    outColor = PackColor(ShadeHit(color, hitNormal, direction, hitDistance));
    return true;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint inputBase = dispatchThreadId.x * 80u;
    uint outputBase = dispatchThreadId.x * 32u;
    float ndcX = LoadFloat(inputBase + 0u);
    float ndcY = LoadFloat(inputBase + 4u);
    uint cellId = SampleInput.Load(inputBase + 12u);
    float3 forward = float3(LoadFloat(inputBase + 16u), LoadFloat(inputBase + 20u), LoadFloat(inputBase + 24u));
    float3 right = float3(LoadFloat(inputBase + 28u), LoadFloat(inputBase + 32u), LoadFloat(inputBase + 36u));
    float3 up = float3(LoadFloat(inputBase + 40u), LoadFloat(inputBase + 44u), LoadFloat(inputBase + 48u));
    float aspect = LoadFloat(inputBase + 52u);
    float halfHeight = LoadFloat(inputBase + 56u);
    float3 rayOrigin = float3(LoadFloat(inputBase + 60u), LoadFloat(inputBase + 64u), LoadFloat(inputBase + 68u));

    float3 rayDirection = normalize(forward + (right * (ndcX * aspect * halfHeight)) + (up * (ndcY * halfHeight)));
    float nearest = 80.0f;
    uint sampledColor = 0xFF05080Cu;
    TracePlane(rayOrigin, rayDirection, float3(0.0f, -1.16f, 0.0f), float3(0.0f, 1.0f, 0.0f), 0xFF101820u, nearest, sampledColor);
    TraceBox(rayOrigin, rayDirection, float3(-3.0f, 0.25f, 6.2f), float3(0.45f, 1.1f, 0.45f), 0xFF1F6F8Bu, nearest, sampledColor);
    TraceBox(rayOrigin, rayDirection, float3(2.7f, 0.05f, 5.2f), float3(0.6f, 0.9f, 0.6f), 0xFF8B2E3Eu, nearest, sampledColor);
    TraceBox(rayOrigin, rayDirection, float3(0.0f, 0.55f, 8.3f), float3(2.1f, 0.1f, 0.3f), 0xFFE0D7A4u, nearest, sampledColor);

    SampleOutput.Store(outputBase + 0u, cellId);
    SampleOutput.Store(outputBase + 4u, asuint(rayDirection.x));
    SampleOutput.Store(outputBase + 8u, asuint(rayDirection.y));
    SampleOutput.Store(outputBase + 12u, asuint(rayDirection.z));
    SampleOutput.Store(outputBase + 16u, sampledColor);
}
)";

    float Clamp01(float value)
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    std::uint32_t PackColor(float red, float green, float blue)
    {
        const auto toByte =
            [](float value) -> std::uint32_t
            {
                return static_cast<std::uint32_t>(Clamp01(value) * 255.0f);
            };

        return 0xFF000000u | (toByte(red) << 16U) | (toByte(green) << 8U) | toByte(blue);
    }

    void UnpackColor(std::uint32_t color, float& red, float& green, float& blue)
    {
        red = static_cast<float>((color >> 16U) & 0xFFu) / 255.0f;
        green = static_cast<float>((color >> 8U) & 0xFFu) / 255.0f;
        blue = static_cast<float>(color & 0xFFu) / 255.0f;
    }

}

bool SandboxApp::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    core.LogInfo("SandboxApp: Light ray lab ready.");
    ResetLab();

    core.SetFrameCallback(
        [this](EngineCore& callbackCore)
        {
            UpdateFrame(callbackCore);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void SandboxApp::UpdateFrame(EngineCore& core)
{
    UpdateLogic(core);

    constexpr std::uint32_t clearColor = 0xFF03070Au;
    const FramePrototype frame = BuildLightLabFrame();

    core.ClearFrame(clearColor);
    core.SubmitRenderFrame(frame);
    core.SetWindowOverlayText(BuildOverlay(core));
}

void SandboxApp::UpdateLogic(EngineCore& core)
{
    ++m_state.frameCount;
    m_state.totalTime = core.GetTotalTime();

    const float windowAspectRatio = core.GetWindowAspectRatio();
    if (windowAspectRatio > 0.000001f)
    {
        m_state.aspectRatio = static_cast<double>(windowAspectRatio);
    }

    if (core.WasKeyPressed(kRestartKey))
    {
        if (m_state.hexObservationMode)
        {
            ResetHexObservation();
            core.LogInfo("SandboxApp: hex observation reset.");
        }
        else
        {
            ResetLab();
            core.LogInfo("SandboxApp: Light lab reset.");
        }
        return;
    }

    if (core.WasKeyPressed(kToggleHexObservationKey))
    {
        m_state.hexObservationMode = !m_state.hexObservationMode;
        if (m_state.hexObservationMode)
        {
            ResetHexObservation();
            core.LogInfo("SandboxApp: hex observation mode enabled.");
        }
        else
        {
            core.LogInfo("SandboxApp: hex observation mode disabled.");
        }
    }

    if (m_state.hexObservationMode)
    {
        UpdateCameraMovement(core);
        UpdateHexObservation(core);
        return;
    }

    if (core.WasKeyPressed(kToggleDebugRaysKey))
    {
        m_state.showDebugRays = !m_state.showDebugRays;
    }

    if (core.WasKeyPressed(kDecreaseRaysKey))
    {
        m_state.rayCount = std::max(kMinRayCount, m_state.rayCount / 2U);
    }

    if (core.WasKeyPressed(kIncreaseRaysKey))
    {
        m_state.rayCount = std::min(kMaxRayCount, m_state.rayCount * 2U);
    }

    UpdateCameraMovement(core);
    UpdateLightControls(core);
    UpdateRandomRayPattern();
    UpdateLightRaySystem();
}

void SandboxApp::UpdateCameraMovement(EngineCore& core)
{
    const float moveDistance = kCameraMoveSpeed * static_cast<float>(core.GetDeltaTime());

    if (core.IsKeyDown('W'))
    {
        m_state.camera.MoveForward(moveDistance);
    }

    if (core.IsKeyDown('S'))
    {
        m_state.camera.MoveForward(-moveDistance);
    }

    if (core.IsKeyDown('A'))
    {
        m_state.camera.MoveRight(-moveDistance);
    }

    if (core.IsKeyDown('D'))
    {
        m_state.camera.MoveRight(moveDistance);
    }

    if (core.IsKeyDown('Q'))
    {
        m_state.camera.MoveUp(-moveDistance);
    }

    if (core.IsKeyDown('E'))
    {
        m_state.camera.MoveUp(moveDistance);
    }
}

void SandboxApp::UpdateLightControls(EngineCore& core)
{
    const float moveDistance = kLightMoveSpeed * static_cast<float>(core.GetDeltaTime());

    if (core.IsKeyDown('J'))
    {
        m_state.lightOrigin.x -= moveDistance;
    }

    if (core.IsKeyDown('L'))
    {
        m_state.lightOrigin.x += moveDistance;
    }

    if (core.IsKeyDown('I'))
    {
        m_state.lightOrigin.z += moveDistance;
    }

    if (core.IsKeyDown('K'))
    {
        m_state.lightOrigin.z -= moveDistance;
    }

    if (core.IsKeyDown('U'))
    {
        m_state.lightOrigin.y += moveDistance;
    }

    if (core.IsKeyDown('O'))
    {
        m_state.lightOrigin.y -= moveDistance;
    }
}

void SandboxApp::UpdateRandomRayPattern()
{
    if (m_state.totalTime < m_state.nextRandomRayRefreshTime)
    {
        return;
    }

    ++m_state.randomRaySeed;
    m_state.nextRandomRayRefreshTime = m_state.totalTime + kRandomRayRefreshSeconds;
}

void SandboxApp::ResetHexObservation()
{
    const bool keepMode = m_state.hexObservationMode;
    m_hexObservationCells.clear();
    m_hexObservationTasks.clear();
    m_state.nextHexObservationId = 1U;
    m_state.activeHexObservationId = 0U;
    m_state.nextHexObservationStepTime = 0.0;
    m_state.nextHexObservationRecursionTime = 0.0;
    m_state.hexObservationAllowedRefineDepth = 0U;
    m_state.hexObservationMaxSampledDepth = 0U;
    m_state.hexObservationGpuBatchCount = 0U;
    m_state.hexObservationCpuBatchCount = 0U;
    m_state.hexObservationGpuSampleCount = 0U;
    m_state.hexObservationCpuSampleCount = 0U;
    m_state.hexObservationGpuFallbackCount = 0U;
    m_state.lastHexObservationBatchUsedGpu = false;
    m_state.hexObservationCompleteLogged = false;
    m_state.hexObservationMode = keepMode;

    std::vector<unsigned int> rootGroupIds;
    rootGroupIds.reserve(7U);

    const auto addCell =
        [this](float ndcX, float ndcY, unsigned int clockwiseOrder)
        {
            HexObservationCell cell;
            cell.id = m_state.nextHexObservationId++;
            cell.depth = 0U;
            cell.clockwiseOrder = clockwiseOrder;
            cell.ndcX = ndcX;
            cell.ndcY = ndcY;
            cell.ndcRadius = kHexObservationStartRadius;
            cell.visible =
                std::fabs(ndcX) <= (1.0f + cell.ndcRadius) &&
                std::fabs(ndcY) <= (1.0f + cell.ndcRadius);
            UpdateHexObservationCellScore(cell);
            const unsigned int cellId = cell.id;
            m_hexObservationCells.push_back(cell);
            return cellId;
        };

    rootGroupIds.push_back(addCell(0.0f, 0.0f, 0U));

    const float ringDistance = kHexObservationStartRadius * 1.72f;
    for (unsigned int index = 0U; index < 6U; ++index)
    {
        const float angle = static_cast<float>(index) * 1.04719755f;
        rootGroupIds.push_back(addCell(
            std::sin(angle) * ringDistance,
            std::cos(angle) * ringDistance,
            index + 1U));
    }

    ScheduleHexObservationGroup(rootGroupIds, false);
}

void SandboxApp::UpdateHexObservation(EngineCore& core)
{
    if (m_state.totalTime < m_state.nextHexObservationStepTime)
    {
        return;
    }

    AdvanceHexObservationRecursionGate(core);

    const auto workStart = std::chrono::steady_clock::now();
    const unsigned int stepBudget = BuildHexObservationStepBudget(core);
    const std::size_t sampleBudget = BuildHexObservationSampleBudget(core);
    bool didWork = false;
    unsigned int stepsUsed = 0U;
    while (stepsUsed < stepBudget && StepHexObservation(core, sampleBudget))
    {
        didWork = true;
        ++stepsUsed;
        AdvanceHexObservationRecursionGate(core);

        const std::chrono::duration<double> spent = std::chrono::steady_clock::now() - workStart;
        if (spent.count() >= kHexObservationMainThreadBudgetSeconds)
        {
            break;
        }
    }

    if (didWork || !m_hexObservationTasks.empty())
    {
        m_state.nextHexObservationStepTime = m_state.totalTime + kHexObservationStepSeconds;
    }
}

void SandboxApp::AdvanceHexObservationRecursionGate(EngineCore& core)
{
    if (m_state.hexObservationMaxSampledDepth <= m_state.hexObservationAllowedRefineDepth)
    {
        return;
    }

    if (m_state.nextHexObservationRecursionTime <= 0.0)
    {
        m_state.nextHexObservationRecursionTime =
            m_state.totalTime + kHexObservationRecursionUnlockSeconds;
        return;
    }

    if (m_state.totalTime < m_state.nextHexObservationRecursionTime)
    {
        return;
    }

    ++m_state.hexObservationAllowedRefineDepth;
    m_state.nextHexObservationRecursionTime = 0.0;

    std::ostringstream message;
    message << "[HEX] Allow inner recursion depth=" << m_state.hexObservationAllowedRefineDepth;
    core.LogInfo(message.str());
}

unsigned int SandboxApp::BuildHexObservationStepBudget(const EngineCore& core) const
{
    const unsigned int workerCount = std::max(1U, core.GetJobWorkerCount());
    const unsigned int idleWorkers = std::max(1U, core.GetIdleJobWorkerCount());
    const unsigned int gpuMultiplier = core.SupportsComputeDispatch() ? 2U : 1U;
    const unsigned int pressure =
        (core.GetQueuedJobCount() > 0U || core.GetActiveJobCount() > 0U)
            ? 1U
            : idleWorkers;
    return std::max(1U, std::min(kHexObservationMaxStepsPerFrame, pressure * gpuMultiplier * 4U + workerCount));
}

std::size_t SandboxApp::BuildHexObservationSampleBudget(const EngineCore& core) const
{
    const unsigned int idleWorkers = std::max(1U, core.GetIdleJobWorkerCount());
    const unsigned int gpuMultiplier = core.SupportsComputeDispatch() ? 2U : 1U;
    const std::size_t pressure =
        (core.GetQueuedJobCount() > 0U || core.GetActiveJobCount() > 0U)
            ? 1U
            : static_cast<std::size_t>(idleWorkers);
    return std::max<std::size_t>(
        1U,
        std::min<std::size_t>(
            kHexObservationMaxSamplesPerBatch,
            pressure * static_cast<std::size_t>(gpuMultiplier) * 8U));
}

bool SandboxApp::StepHexObservation(EngineCore& core, std::size_t sampleBudget)
{
    if (ProcessHexObservationSampleBatch(core, sampleBudget))
    {
        return true;
    }

    if (ProcessHexObservationRefineTask(core))
    {
        return true;
    }

    if (!m_hexObservationTasks.empty())
    {
        return false;
    }

    if (!m_state.hexObservationCompleteLogged)
    {
        core.LogInfo("[HEX] Observation queue drained.");
        m_state.hexObservationCompleteLogged = true;
    }

    return false;
}

bool SandboxApp::ProcessHexObservationSampleBatch(EngineCore& core, std::size_t sampleBudget)
{
    std::vector<unsigned int> sampleCellIds;
    sampleBudget = std::max<std::size_t>(1U, std::min<std::size_t>(sampleBudget, kHexObservationMaxSamplesPerBatch));
    sampleCellIds.reserve(sampleBudget);

    while (!m_hexObservationTasks.empty() && sampleCellIds.size() < sampleBudget)
    {
        const HexObservationTask task = m_hexObservationTasks.front();
        if (task.kind != HexObservationTaskKind::Sample)
        {
            break;
        }

        m_hexObservationTasks.pop_front();
        const HexObservationCell* cell = FindHexObservationCell(task.cellId);
        if (!cell || !cell->visible || cell->sampled)
        {
            continue;
        }

        sampleCellIds.push_back(task.cellId);
    }

    if (sampleCellIds.empty())
    {
        return false;
    }

    if (core.SupportsComputeDispatch())
    {
        std::vector<HexObservationGpuSampleInput> gpuInputs(sampleCellIds.size());
        std::vector<HexObservationGpuSampleOutput> gpuOutputs(sampleCellIds.size());
        const Vector3 forward = m_state.camera.GetForward();
        const Vector3 right = m_state.camera.GetRight();
        const Vector3 up = m_state.camera.GetUpDirection();
        const float fovRadians = m_state.camera.fovDegrees * 0.01745329252f;
        const float halfHeight = std::tan(fovRadians * 0.5f);
        const float aspect = static_cast<float>(m_state.aspectRatio);

        for (std::size_t index = 0U; index < sampleCellIds.size(); ++index)
        {
            const HexObservationCell* cell = FindHexObservationCell(sampleCellIds[index]);
            if (!cell)
            {
                continue;
            }

            HexObservationGpuSampleInput input;
            input.ndcX = cell->ndcX;
            input.ndcY = cell->ndcY;
            input.depth = cell->depth;
            input.cellId = cell->id;
            input.forwardX = forward.x;
            input.forwardY = forward.y;
            input.forwardZ = forward.z;
            input.rightX = right.x;
            input.rightY = right.y;
            input.rightZ = right.z;
            input.upX = up.x;
            input.upY = up.y;
            input.upZ = up.z;
            input.aspect = aspect;
            input.halfHeight = halfHeight;
            input.originX = m_state.camera.position.x;
            input.originY = m_state.camera.position.y;
            input.originZ = m_state.camera.position.z;
            gpuInputs[index] = input;
        }

        GpuTaskDesc gpuTask;
        gpuTask.name = "HexObservationSample";
        gpuTask.shaderSource = kHexObservationGpuSampleShader;
        gpuTask.entryPoint = "CSMain";
        gpuTask.input.data = reinterpret_cast<const std::byte*>(gpuInputs.data());
        gpuTask.input.size = gpuInputs.size() * sizeof(HexObservationGpuSampleInput);
        gpuTask.output.data = reinterpret_cast<std::byte*>(gpuOutputs.data());
        gpuTask.output.size = gpuOutputs.size() * sizeof(HexObservationGpuSampleOutput);
        gpuTask.dispatchX = static_cast<std::uint32_t>(gpuInputs.size());

        if (core.SubmitGpuTask(gpuTask))
        {
            ++m_state.hexObservationGpuBatchCount;
            m_state.hexObservationGpuSampleCount += static_cast<unsigned long long>(gpuOutputs.size());
            m_state.lastHexObservationBatchUsedGpu = true;

            for (const HexObservationGpuSampleOutput& output : gpuOutputs)
            {
                if (output.cellId != 0U)
                {
                    HexObservationSampleResult result;
                    result.cellId = output.cellId;
                    result.rayDirection = Normalize(Vector3(output.rayX, output.rayY, output.rayZ));
                    result.sampleColor = output.sampleColor;
                    ApplyHexObservationSample(result, core);
                }
            }

            return true;
        }

        ++m_state.hexObservationGpuFallbackCount;
    }

    ++m_state.hexObservationCpuBatchCount;
    m_state.hexObservationCpuSampleCount += static_cast<unsigned long long>(sampleCellIds.size());
    m_state.lastHexObservationBatchUsedGpu = false;

    std::vector<HexObservationSampleResult> results(sampleCellIds.size());
    for (std::size_t index = 0U; index < sampleCellIds.size(); ++index)
    {
        const unsigned int cellId = sampleCellIds[index];
        const HexObservationCell* cell = FindHexObservationCell(cellId);
        if (!cell)
        {
            continue;
        }

        const float ndcX = cell->ndcX;
        const float ndcY = cell->ndcY;
        const unsigned int depth = cell->depth;
        core.SubmitJob(
            [this, &results, index, cellId, ndcX, ndcY, depth]()
            {
                HexObservationSampleResult result;
                result.cellId = cellId;
                result.rayDirection = BuildHexObservationRay(ndcX, ndcY);
                result.sampleColor = BuildHexObservationSampleColor(result.rayDirection, depth);
                results[index] = result;
            });
    }

    core.WaitForAllJobs();

    for (const HexObservationSampleResult& result : results)
    {
        if (result.cellId != 0U)
        {
            ApplyHexObservationSample(result, core);
        }
    }

    return true;
}

bool SandboxApp::ProcessHexObservationRefineTask(EngineCore& core)
{
    auto bestTaskIt = m_hexObservationTasks.end();
    float bestScore = -1000000.0f;
    bool hasPendingParentRefine = false;

    for (const HexObservationTask& task : m_hexObservationTasks)
    {
        if (task.kind != HexObservationTaskKind::Refine)
        {
            continue;
        }

        const HexObservationCell* cell = FindHexObservationCell(task.cellId);
        if (cell &&
            cell->parentId == 0U &&
            cell->visible &&
            cell->sampled &&
            !cell->subdivided &&
            !cell->settled &&
            cell->depth <= m_state.hexObservationAllowedRefineDepth)
        {
            hasPendingParentRefine = true;
            break;
        }
    }

    for (auto it = m_hexObservationTasks.begin(); it != m_hexObservationTasks.end();)
    {
        const HexObservationTask task = *it;
        if (task.kind == HexObservationTaskKind::Sample)
        {
            ++it;
            continue;
        }

        HexObservationCell* cell = FindHexObservationCell(task.cellId);
        if (!cell || !cell->visible || !cell->sampled || cell->subdivided || cell->settled)
        {
            it = m_hexObservationTasks.erase(it);
            continue;
        }

        if (hasPendingParentRefine && cell->parentId != 0U)
        {
            ++it;
            continue;
        }

        if (cell->depth > m_state.hexObservationAllowedRefineDepth)
        {
            ++it;
            continue;
        }

        UpdateHexObservationCellScore(*cell);
        const float score = cell->priorityScore - (static_cast<float>(cell->clockwiseOrder) * 0.0001f);
        if (score > bestScore)
        {
            bestScore = score;
            bestTaskIt = it;
        }

        ++it;
    }

    if (bestTaskIt == m_hexObservationTasks.end())
    {
        return false;
    }

    const HexObservationTask task = *bestTaskIt;
    m_hexObservationTasks.erase(bestTaskIt);

    HexObservationCell* cell = FindHexObservationCell(task.cellId);
    if (!cell || !cell->visible || !cell->sampled || cell->subdivided || cell->settled)
    {
        return true;
    }

    const std::vector<unsigned int> childGroupIds = SubdivideHexObservationCell(*cell, core);
    ScheduleHexObservationGroup(childGroupIds, true);
    return true;
}

SandboxApp::HexObservationCell* SandboxApp::FindHexObservationCell(unsigned int id)
{
    for (HexObservationCell& cell : m_hexObservationCells)
    {
        if (cell.id == id)
        {
            return &cell;
        }
    }

    return nullptr;
}

const SandboxApp::HexObservationCell* SandboxApp::FindHexObservationCell(unsigned int id) const
{
    for (const HexObservationCell& cell : m_hexObservationCells)
    {
        if (cell.id == id)
        {
            return &cell;
        }
    }

    return nullptr;
}

void SandboxApp::ScheduleHexObservationGroup(const std::vector<unsigned int>& cellIds, bool sampleAtFront)
{
    if (cellIds.empty())
    {
        return;
    }

    std::vector<HexObservationTask> sampleTasks;
    sampleTasks.reserve(cellIds.size());

    for (unsigned int cellId : cellIds)
    {
        sampleTasks.push_back(HexObservationTask{ HexObservationTaskKind::Sample, cellId });
    }

    if (sampleAtFront)
    {
        for (auto it = sampleTasks.rbegin(); it != sampleTasks.rend(); ++it)
        {
            m_hexObservationTasks.push_front(*it);
        }
    }
    else
    {
        for (const HexObservationTask& task : sampleTasks)
        {
            m_hexObservationTasks.push_back(task);
        }
    }

    for (unsigned int cellId : cellIds)
    {
        m_hexObservationTasks.push_back(HexObservationTask{ HexObservationTaskKind::Refine, cellId });
    }
}

void SandboxApp::UpdateHexObservationCellScore(HexObservationCell& cell)
{
    cell.difficultyScore = ComputeHexObservationCellDifficulty(cell);
    cell.priorityScore = ComputeHexObservationCellPriority(cell);
}

float SandboxApp::ComputeHexObservationCellPriority(const HexObservationCell& cell) const
{
    const float centerDistance = std::sqrt((cell.ndcX * cell.ndcX) + (cell.ndcY * cell.ndcY));
    const float centerBias = 1.0f / (1.0f + (centerDistance * 2.75f));
    const float depthCost = static_cast<float>(cell.depth) * 0.035f;
    const float parentBias = cell.parentId == 0U ? 0.2f : 0.0f;
    const float freshBias = cell.childPassCount == 0U ? 0.18f : 0.0f;
    const float visibilityBias = cell.visible ? 0.15f : -10.0f;
    const float settlePenalty = cell.settled ? 10.0f : 0.0f;
    return (centerBias * 2.4f) +
        (cell.difficultyScore * 2.1f) +
        parentBias +
        freshBias +
        visibilityBias -
        depthCost -
        settlePenalty;
}

float SandboxApp::ComputeHexObservationCellDifficulty(const HexObservationCell& cell) const
{
    if (!cell.sampled)
    {
        return 1.0f;
    }

    float difficulty = 0.08f;
    const float centerDistance = std::sqrt((cell.ndcX * cell.ndcX) + (cell.ndcY * cell.ndcY));
    difficulty += 0.18f / (1.0f + (centerDistance * 3.0f));

    const HexObservationCell* parent = FindHexObservationCell(cell.parentId);
    if (parent && parent->aggregateSampleCount > 0U)
    {
        difficulty += ComputeHexObservationColorDifference(
            cell.sampleColor,
            parent->aggregateRed,
            parent->aggregateGreen,
            parent->aggregateBlue) * 1.4f;
    }

    if (cell.childPassCount > 0U)
    {
        const float settling = std::min(0.45f, static_cast<float>(cell.childPassCount) * 0.055f);
        difficulty = std::max(0.0f, difficulty - settling);
    }

    return std::max(0.0f, std::min(1.0f, difficulty));
}

float SandboxApp::ComputeHexObservationColorDifference(
    std::uint32_t firstColor,
    float secondRed,
    float secondGreen,
    float secondBlue) const
{
    float firstRed = 0.0f;
    float firstGreen = 0.0f;
    float firstBlue = 0.0f;
    UnpackColor(firstColor, firstRed, firstGreen, firstBlue);
    const float redDifference = firstRed - secondRed;
    const float greenDifference = firstGreen - secondGreen;
    const float blueDifference = firstBlue - secondBlue;
    return std::sqrt(
        (redDifference * redDifference) +
        (greenDifference * greenDifference) +
        (blueDifference * blueDifference)) / 1.7320508f;
}

void SandboxApp::ApplyHexObservationSample(const HexObservationSampleResult& result, EngineCore& core)
{
    HexObservationCell* cell = FindHexObservationCell(result.cellId);
    if (!cell || !cell->visible || cell->sampled)
    {
        return;
    }

    for (HexObservationCell& other : m_hexObservationCells)
    {
        other.active = false;
    }

    cell->active = true;
    cell->sampled = true;
    cell->rayDirection = result.rayDirection;
    cell->sampleColor = result.sampleColor;
    UnpackColor(cell->sampleColor, cell->aggregateRed, cell->aggregateGreen, cell->aggregateBlue);
    cell->aggregateSampleCount = std::max(1U, cell->aggregateSampleCount);
    ++cell->samplePassCount;
    cell->difficultyScore = ComputeHexObservationCellDifficulty(*cell);
    cell->settled =
        cell->parentId != 0U &&
        cell->difficultyScore < kHexObservationSettleDifficulty &&
        cell->depth > 1U;
    UpdateHexObservationCellScore(*cell);
    m_state.activeHexObservationId = cell->id;
    m_state.hexObservationMaxSampledDepth = std::max(m_state.hexObservationMaxSampledDepth, cell->depth);
    m_state.hexObservationCompleteLogged = false;

    std::ostringstream message;
    message << "[HEX] Select id=" << cell->id
            << " depth=" << cell->depth
            << (cell->parentId == 0U ? " parent pass" : " child pass")
            << " difficulty=" << cell->difficultyScore
            << " priority=" << cell->priorityScore
            << (cell->settled ? " settled" : "");
    core.LogInfo(message.str());

    if (cell->parentId != 0U)
    {
        UpdateParentHexObservation(*cell, core);
    }
}

std::vector<unsigned int> SandboxApp::SubdivideHexObservationCell(HexObservationCell& cell, EngineCore& core)
{
    std::vector<unsigned int> childGroupIds;
    childGroupIds.reserve(7U);

    for (HexObservationCell& other : m_hexObservationCells)
    {
        other.active = false;
    }

    const unsigned int parentId = cell.id;
    const unsigned int childDepth = cell.depth + 1U;
    const float parentNdcX = cell.ndcX;
    const float parentNdcY = cell.ndcY;
    const float childRadius = cell.ndcRadius * kHexObservationChildRadiusScale;
    const float childDistance = childRadius * 1.72f;

    cell.subdivided = true;
    cell.active = true;
    m_state.activeHexObservationId = cell.id;
    m_state.hexObservationCompleteLogged = false;

    std::ostringstream message;
    message << "[HEX] Subdivide id=" << cell.id
            << " depth=" << cell.depth;
    core.LogInfo(message.str());

    const auto addChild =
        [this, parentId, childRadius, childDepth](float ndcX, float ndcY, unsigned int clockwiseOrder)
        {
            HexObservationCell child;
            child.id = m_state.nextHexObservationId++;
            child.parentId = parentId;
            child.depth = childDepth;
            child.clockwiseOrder = clockwiseOrder;
            child.ndcX = ndcX;
            child.ndcY = ndcY;
            child.ndcRadius = childRadius;
            child.visible =
                std::fabs(ndcX) <= (1.0f + childRadius) &&
                std::fabs(ndcY) <= (1.0f + childRadius);
            UpdateHexObservationCellScore(child);
            const unsigned int childId = child.id;
            m_hexObservationCells.push_back(child);
            return childId;
        };

    childGroupIds.push_back(addChild(parentNdcX, parentNdcY, 0U));
    for (unsigned int index = 0U; index < 6U; ++index)
    {
        const float angle = static_cast<float>(index) * 1.04719755f;
        childGroupIds.push_back(addChild(
            parentNdcX + (std::sin(angle) * childDistance),
            parentNdcY + (std::cos(angle) * childDistance),
            index + 1U));
    }

    return childGroupIds;
}

void SandboxApp::UpdateParentHexObservation(const HexObservationCell& child, EngineCore& core)
{
    HexObservationCell* parent = FindHexObservationCell(child.parentId);
    if (!parent)
    {
        return;
    }

    float childRed = 0.0f;
    float childGreen = 0.0f;
    float childBlue = 0.0f;
    UnpackColor(child.sampleColor, childRed, childGreen, childBlue);

    const float currentCount = static_cast<float>(parent->aggregateSampleCount);
    parent->aggregateRed = ((parent->aggregateRed * currentCount) + childRed) / (currentCount + 1.0f);
    parent->aggregateGreen = ((parent->aggregateGreen * currentCount) + childGreen) / (currentCount + 1.0f);
    parent->aggregateBlue = ((parent->aggregateBlue * currentCount) + childBlue) / (currentCount + 1.0f);
    ++parent->aggregateSampleCount;
    ++parent->childPassCount;
    parent->difficultyScore = ComputeHexObservationCellDifficulty(*parent);
    parent->settled =
        parent->parentId != 0U &&
        parent->childPassCount >= 3U &&
        parent->difficultyScore < kHexObservationSettleDifficulty;
    UpdateHexObservationCellScore(*parent);

    std::ostringstream message;
    message << "[HEX] Update parent id=" << parent->id
            << " from child id=" << child.id
            << " samples=" << parent->aggregateSampleCount
            << " difficulty=" << parent->difficultyScore
            << (parent->settled ? " settled" : "");
    core.LogInfo(message.str());
}

Vector3 SandboxApp::BuildHexObservationRay(float ndcX, float ndcY) const
{
    const Vector3 forward = m_state.camera.GetForward();
    const Vector3 right = m_state.camera.GetRight();
    const Vector3 up = m_state.camera.GetUpDirection();
    const float fovRadians = m_state.camera.fovDegrees * 0.01745329252f;
    const float halfHeight = std::tan(fovRadians * 0.5f);
    const float aspect = static_cast<float>(m_state.aspectRatio);

    return Normalize(
        forward +
        (right * (ndcX * aspect * halfHeight)) +
        (up * (ndcY * halfHeight)));
}

std::uint32_t SandboxApp::BuildHexObservationSampleColor(const Vector3& rayDirection, unsigned int depth) const
{
    (void)depth;

    const Vector3 rayOrigin = m_state.camera.position;
    float nearestDistance = 80.0f;
    std::uint32_t sampledColor = 0xFF05080Cu;

    TrySampleHexObservationPlane(
        rayOrigin,
        rayDirection,
        Vector3(0.0f, -1.16f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f),
        0xFF101820u,
        nearestDistance,
        sampledColor);

    TrySampleHexObservationBox(
        rayOrigin,
        rayDirection,
        Vector3(-3.0f, 0.25f, 6.2f),
        Vector3(0.45f, 1.1f, 0.45f),
        0xFF1F6F8Bu,
        nearestDistance,
        sampledColor);

    TrySampleHexObservationBox(
        rayOrigin,
        rayDirection,
        Vector3(2.7f, 0.05f, 5.2f),
        Vector3(0.6f, 0.9f, 0.6f),
        0xFF8B2E3Eu,
        nearestDistance,
        sampledColor);

    TrySampleHexObservationBox(
        rayOrigin,
        rayDirection,
        Vector3(0.0f, 0.55f, 8.3f),
        Vector3(2.1f, 0.1f, 0.3f),
        0xFFE0D7A4u,
        nearestDistance,
        sampledColor);

    return sampledColor;
}

bool SandboxApp::TrySampleHexObservationBox(
    const Vector3& rayOrigin,
    const Vector3& rayDirection,
    const Vector3& center,
    const Vector3& halfExtents,
    std::uint32_t color,
    float& inOutNearestDistance,
    std::uint32_t& outColor) const
{
    const Vector3 minCorner = center - halfExtents;
    const Vector3 maxCorner = center + halfExtents;
    float tMin = 0.0f;
    float tMax = inOutNearestDistance;
    Vector3 hitNormal = Vector3(0.0f, 1.0f, 0.0f);

    const auto traceAxis =
        [&](float origin, float direction, float minValue, float maxValue, const Vector3& axisNormal) -> bool
        {
            constexpr float kMinimumDirectionLength = 0.000001f;
            if (std::fabs(direction) <= kMinimumDirectionLength)
            {
                return origin >= minValue && origin <= maxValue;
            }

            float nearDistance = (minValue - origin) / direction;
            float farDistance = (maxValue - origin) / direction;
            Vector3 nearNormal = axisNormal * -1.0f;

            if (nearDistance > farDistance)
            {
                std::swap(nearDistance, farDistance);
                nearNormal = axisNormal;
            }

            if (nearDistance > tMin)
            {
                tMin = nearDistance;
                hitNormal = nearNormal;
            }

            tMax = std::min(tMax, farDistance);
            return tMin <= tMax;
        };

    if (!traceAxis(rayOrigin.x, rayDirection.x, minCorner.x, maxCorner.x, Vector3(1.0f, 0.0f, 0.0f)) ||
        !traceAxis(rayOrigin.y, rayDirection.y, minCorner.y, maxCorner.y, Vector3(0.0f, 1.0f, 0.0f)) ||
        !traceAxis(rayOrigin.z, rayDirection.z, minCorner.z, maxCorner.z, Vector3(0.0f, 0.0f, 1.0f)))
    {
        return false;
    }

    const float hitDistance = tMin >= 0.0f ? tMin : tMax;
    if (hitDistance < 0.0f || hitDistance >= inOutNearestDistance)
    {
        return false;
    }

    inOutNearestDistance = hitDistance;
    outColor = ShadeHexObservationHit(color, hitNormal, rayDirection, hitDistance);
    return true;
}

bool SandboxApp::TrySampleHexObservationPlane(
    const Vector3& rayOrigin,
    const Vector3& rayDirection,
    const Vector3& point,
    const Vector3& normal,
    std::uint32_t color,
    float& inOutNearestDistance,
    std::uint32_t& outColor) const
{
    constexpr float kMinimumDirectionLength = 0.000001f;
    const Vector3 normalizedNormal = Normalize(normal);
    const float denominator = Dot(rayDirection, normalizedNormal);
    if (std::fabs(denominator) <= kMinimumDirectionLength)
    {
        return false;
    }

    const float distance = Dot(point - rayOrigin, normalizedNormal) / denominator;
    if (distance < 0.0f || distance >= inOutNearestDistance)
    {
        return false;
    }

    const Vector3 facingNormal = denominator > 0.0f ? (normalizedNormal * -1.0f) : normalizedNormal;
    inOutNearestDistance = distance;
    outColor = ShadeHexObservationHit(color, facingNormal, rayDirection, distance);
    return true;
}

std::uint32_t SandboxApp::ShadeHexObservationHit(
    std::uint32_t baseColor,
    const Vector3& hitNormal,
    const Vector3& rayDirection,
    float distance) const
{
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    UnpackColor(baseColor, red, green, blue);

    const Vector3 lightDirection = Normalize(Vector3(-0.35f, 0.8f, -0.4f));
    const float diffuse = std::max(0.0f, Dot(Normalize(hitNormal), lightDirection));
    const float facing = 0.35f + (std::max(0.0f, Dot(Normalize(hitNormal), rayDirection * -1.0f)) * 0.25f);
    const float distanceFade = 1.0f / (1.0f + (distance * 0.035f));
    const float shade = (0.28f + (diffuse * 0.52f) + facing) * distanceFade;

    return PackColor(red * shade, green * shade, blue * shade);
}

std::uint32_t SandboxApp::BuildHexObservationAggregateColor(const HexObservationCell& cell) const
{
    if (cell.aggregateSampleCount == 0U)
    {
        return cell.sampleColor;
    }

    return PackColor(cell.aggregateRed, cell.aggregateGreen, cell.aggregateBlue);
}

void SandboxApp::ResetLab()
{
    m_state = SandboxState{};
    m_state.camera = CameraPrototype{};
    m_state.camera.position = Vector3(0.0f, 2.2f, -10.0f);
    m_state.camera.LookAt(Vector3(0.0f, 0.8f, 5.0f));
    m_state.camera.SetPerspective(58.0f, 0.1f, 120.0f);
    m_state.nextRandomRayRefreshTime = 0.0;
    m_hexObservationCells.clear();
    UpdateLightRaySystem();
}

LightRayEmitterPrototype SandboxApp::BuildLightEmitter() const
{
    Vector3 lightDirection = Normalize(m_state.lightDirection);

    LightRayEmitterPrototype labEmitter;
    labEmitter.origin = m_state.lightOrigin;
    labEmitter.SetDirection(lightDirection);
    labEmitter.color = ColorPrototype::FromBytes(180, 225, 255);
    labEmitter.intensity = 1.0f;
    labEmitter.rayCount = m_state.rayCount;
    labEmitter.maxBounceCount = 1U;
    labEmitter.randomSeed = m_state.randomRaySeed;
    labEmitter.spreadAngleRadians = m_state.spreadAngleRadians;
    labEmitter.maxDistance = m_state.maxDistance;
    return labEmitter;
}

void SandboxApp::UpdateLightRaySystem()
{
    m_lightRaySystem.ClearEmitters();
    m_lightRaySystem.ClearTraceShapes();
    m_lightRaySystem.AddEmitter(BuildLightEmitter());
    ConfigureLightRayTraceScene();
    m_lightRaySystem.Update();
}

void SandboxApp::ConfigureLightRayTraceScene()
{
    LightRayPlaneTraceShape floor;
    floor.point = Vector3(0.0f, -1.16f, 0.0f);
    floor.normal = Vector3(0.0f, 1.0f, 0.0f);
    floor.material.reflectivity = 0.55f;
    floor.material.absorption = 0.15f;
    m_lightRaySystem.AddPlane(floor);

    LightRayBoxTraceShape leftBlock;
    leftBlock.center = Vector3(-3.0f, 0.25f, 6.2f);
    leftBlock.halfExtents = Vector3(0.45f, 1.1f, 0.45f);
    leftBlock.material.reflectivity = 0.25f;
    leftBlock.material.absorption = 0.35f;
    m_lightRaySystem.AddBox(leftBlock);

    LightRayBoxTraceShape rightBlock;
    rightBlock.center = Vector3(2.7f, 0.05f, 5.2f);
    rightBlock.halfExtents = Vector3(0.6f, 0.9f, 0.6f);
    rightBlock.material.reflectivity = 0.8f;
    rightBlock.material.absorption = 0.05f;
    m_lightRaySystem.AddBox(rightBlock);

    LightRayBoxTraceShape bar;
    bar.center = Vector3(0.0f, 0.55f, 8.3f);
    bar.halfExtents = Vector3(2.1f, 0.1f, 0.3f);
    bar.material.reflectivity = 0.65f;
    bar.material.absorption = 0.2f;
    m_lightRaySystem.AddBox(bar);
}

FramePrototype SandboxApp::BuildLightLabFrame()
{
    ViewPrototype view;
    view.kind = ViewKind::Scene3D;
    view.camera = m_state.camera;

    if (m_state.hexObservationMode)
    {
        view.kind = ViewKind::Overlay2D;
        view.items.reserve(m_hexObservationCells.size());
        for (const HexObservationCell& cell : m_hexObservationCells)
        {
            if (cell.visible && cell.sampled)
            {
                AddHexObservationPatch(view, cell);
            }
        }

        PassPrototype pass;
        pass.kind = PassKind::Scene;
        pass.views.push_back(std::move(view));

        FramePrototype frame;
        frame.passes.push_back(std::move(pass));
        return frame;
    }

    view.lightRayEmitters.reserve(1);
    view.items.reserve(m_lightRaySystem.GetTraceResultCount() + 8U);

    const LightRayEmitterPrototype labEmitter = BuildLightEmitter();
    view.lightRayEmitters.push_back(labEmitter);

    AddCube(view, Vector3(0.0f, -1.2f, 5.0f), Vector3(9.0f, 0.08f, 12.0f), Vector3(0.0f, 0.0f, 0.0f), 0xFF101820u);
    AddCube(view, Vector3(-3.0f, 0.25f, 6.2f), Vector3(0.9f, 2.2f, 0.9f), Vector3(0.0f, 0.2f, 0.0f), 0xFF1F6F8Bu);
    AddCube(view, Vector3(2.7f, 0.05f, 5.2f), Vector3(1.2f, 1.8f, 1.2f), Vector3(0.0f, -0.45f, 0.0f), 0xFF8B2E3Eu);
    AddCube(view, Vector3(0.0f, 0.55f, 8.3f), Vector3(4.2f, 0.2f, 0.6f), Vector3(0.0f, 0.0f, 0.0f), 0xFFE0D7A4u);
    AddCube(view, m_state.lightOrigin, Vector3(0.28f, 0.28f, 0.28f), Vector3(0.0f, 0.0f, 0.0f), 0xFFFFF2A6u);

    for (const LightRayTraceResult& result : m_lightRaySystem.GetTraceResults())
    {
        if (result.DidHit())
        {
            AddCube(
                view,
                result.hitPosition,
                Vector3(0.12f, 0.12f, 0.12f),
                Vector3(0.0f, 0.0f, 0.0f),
                0xFFFFF2A6u);
        }
    }

    if (m_state.showDebugRays)
    {
        for (const LightRayTraceResult& result : m_lightRaySystem.GetTraceResults())
        {
            const std::uint32_t rayColor =
                result.emittedRay.bounceDepth > 0U
                    ? (result.DidHit() ? 0xFFFFA7C7u : 0xFFFF70A6u)
                    : (result.DidHit() ? 0xFFFFF2A6u : 0xFFB7F7FFu);
            const float rayLength = result.DidHit() ? result.hitDistance : result.emittedRay.maxDistance;
            AddRayMarker(
                view,
                result.emittedRay.origin,
                result.emittedRay.direction,
                rayLength,
                0.025f,
                rayColor);
        }
    }

    PassPrototype pass;
    pass.kind = PassKind::Scene;
    pass.views.push_back(std::move(view));

    FramePrototype frame;
    frame.passes.push_back(std::move(pass));
    return frame;
}

std::wstring SandboxApp::BuildOverlay(const EngineCore& core) const
{
    if (m_state.hexObservationMode)
    {
        return
            L"HEX OBSERVATION TEST\n"
            L"  Submitted Items: " + std::to_wstring(core.GetRenderSubmittedItemCount()) + L"\n"
            L"  Cells: " + std::to_wstring(m_hexObservationCells.size()) + L"\n"
            L"  Active Hex: " + std::to_wstring(m_state.activeHexObservationId) + L"\n"
            L"  Refine Depth: " + std::to_wstring(m_state.hexObservationAllowedRefineDepth) + L"\n"
            L"  Worker Budget: " + std::to_wstring(BuildHexObservationSampleBudget(core)) + L" samples\n"
            L"  Workers Idle: " + std::to_wstring(core.GetIdleJobWorkerCount()) + L" / " + std::to_wstring(core.GetJobWorkerCount()) + L"\n"
            L"  GPU Compute: " + std::wstring(core.SupportsComputeDispatch() ? L"On" : L"Stub") + L"\n"
            L"  Last Batch: " + std::wstring(m_state.lastHexObservationBatchUsedGpu ? L"GPU" : L"CPU") + L"\n"
            L"  GPU Batches: " + std::to_wstring(m_state.hexObservationGpuBatchCount) + L" / samples " + std::to_wstring(m_state.hexObservationGpuSampleCount) + L"\n"
            L"  CPU Batches: " + std::to_wstring(m_state.hexObservationCpuBatchCount) + L" / samples " + std::to_wstring(m_state.hexObservationCpuSampleCount) + L"\n"
            L"  GPU Fallbacks: " + std::to_wstring(m_state.hexObservationGpuFallbackCount) + L"\n"
            L"  H: toggle hex observation\n"
            L"  R: reset hex observation\n"
            L"  WASD/QE: move camera\n"
            L"  Escape: exit\n";
    }

    return
        L"LIGHT RAY LAB\n"
        L"  Submitted Items: " + std::to_wstring(core.GetRenderSubmittedItemCount()) + L"\n"
        L"  Ray Count: " + std::to_wstring(m_state.rayCount) + L"\n"
        L"  System Rays: " + std::to_wstring(m_lightRaySystem.GetTraceResultCount()) + L"\n"
        L"  Hits: " + std::to_wstring(CountLightRayHits()) + L"\n"
        L"  Trace Shapes: " + std::to_wstring(m_lightRaySystem.GetTraceShapeCount()) + L"\n"
        L"  Ray Seed: " + std::to_wstring(m_state.randomRaySeed) + L"\n"
        L"  Debug Rays: " + std::wstring(m_state.showDebugRays ? L"On" : L"Off") + L"\n"
        L"  Max Distance: " + std::to_wstring(m_state.maxDistance) + L"\n"
        L"  WASD/QE: move camera\n"
        L"  IJKL/UO: move light source\n"
        L"  [/]: halve/double ray count\n"
        L"  V: toggle debug rays\n"
        L"  H: toggle hex observation\n"
        L"  R: reset\n"
        L"  Escape: exit\n";
}

void SandboxApp::AddCube(
    ViewPrototype& view,
    const Vector3& position,
    const Vector3& scale,
    const Vector3& rotation,
    std::uint32_t color) const
{
    ItemPrototype item;
    item.kind = ItemKind::Object3D;
    item.object3D.GetPrimaryMesh().builtInKind = BuiltInMeshKind::Cube;
    item.object3D.transform.translation = position;
    item.object3D.transform.scale = scale;
    item.object3D.transform.rotationRadians = rotation;
    item.object3D.GetPrimaryMesh().material.baseColor = ColorPrototype::FromArgb(color);
    view.items.push_back(item);
}

void SandboxApp::AddRayMarker(
    ViewPrototype& view,
    const Vector3& origin,
    const Vector3& direction,
    float length,
    float thickness,
    std::uint32_t color) const
{
    const Vector3 normalizedDirection = Normalize(direction);
    const Vector3 midpoint = origin + (normalizedDirection * (length * 0.5f));
    const float yaw = std::atan2(normalizedDirection.x, normalizedDirection.z);
    const float clampedY = std::max(-1.0f, std::min(1.0f, normalizedDirection.y));
    const float pitch = -std::asin(clampedY);
    AddCube(
        view,
        midpoint,
        Vector3(thickness, thickness, length),
        Vector3(pitch, yaw, 0.0f),
        color);
}

void SandboxApp::AddHexObservationPatch(ViewPrototype& view, const HexObservationCell& cell) const
{
    const float aspect = static_cast<float>(m_state.aspectRatio);
    ItemPrototype item;
    item.kind = ItemKind::ScreenCell;
    item.screenCell.centerX = cell.ndcX;
    item.screenCell.centerY = cell.ndcY;
    item.screenCell.halfWidth = std::max(0.012f, cell.ndcRadius * 0.72f);
    item.screenCell.halfHeight = std::max(0.012f, cell.ndcRadius * 0.72f * aspect);
    item.screenCell.color = BuildHexObservationAggregateColor(cell);
    view.items.push_back(item);
}

unsigned int SandboxApp::CountLightRayHits() const
{
    unsigned int hitCount = 0U;
    for (const LightRayTraceResult& result : m_lightRaySystem.GetTraceResults())
    {
        if (result.DidHit())
        {
            ++hitCount;
        }
    }

    return hitCount;
}
