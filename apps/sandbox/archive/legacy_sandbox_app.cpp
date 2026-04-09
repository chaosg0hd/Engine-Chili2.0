#include "sandbox_app.hpp"

#include "core/engine_core.hpp"
#include "prototypes/render/render_frame.hpp"
#include "prototypes/render/render_item.hpp"
#include "prototypes/render/render_pass.hpp"
#include "prototypes/render/render_view.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace
{
    struct TestData
    {
        int value;
        double weight;

        TestData(int v, double w)
            : value(v), weight(w)
        {
        }
    };

    int SumArray(const int* values, int count)
    {
        int total = 0;

        for (int index = 0; index < count; ++index)
        {
            total += values[index];
        }

        return total;
    }
}

bool SandboxApp::Run()
{
    EngineCore core;
    m_featureChecks.clear();

    if (!core.Initialize())
    {
        return false;
    }

    core.LogInfo(
        std::string("SandboxApp: Logger ready | file logging = ") +
        (core.IsFileLoggingAvailable() ? "true" : "false") +
        " | log path = " +
        core.GetLogFilePath()
    );

    bool success = ExecuteFeatureTest(core, "Startup", &SandboxApp::RunStartupChecks);
    success = success && ExecuteFeatureTest(core, "Memory", &SandboxApp::RunMemoryFeatureTest);
    success = success && ExecuteFeatureTest(core, "Raw Memory", &SandboxApp::RunRawMemoryFeatureTest);
    success = success && ExecuteFeatureTest(core, "File", &SandboxApp::RunFileFeatureTest);
    success = success && ExecuteFeatureTest(core, "GPU Compute", &SandboxApp::RunGpuFeatureTest);
    success = success && ExecuteFeatureTest(core, "Jobs", &SandboxApp::RunJobFeatureTest);
    success = success && ExecuteFeatureTest(core, "Resources", &SandboxApp::RunResourceFeatureTest);
    success = success && ExecuteFeatureTest(core, "Frame Prototype", &SandboxApp::RunFramePrototypeFeatureTest);
    success = success && ExecuteFeatureTest(core, "Input", &SandboxApp::RunInputFeatureTest);

    if (success)
    {
        LogFeatureResultSummary(core);
        LogFeatureSummary(core);
        core.SetFrameCallback(
            [this](EngineCore& callbackCore)
            {
                UpdateFrame(callbackCore);
            });
        core.LogInfo("SandboxApp: Entering runtime loop. Close the window or press Escape to exit.");
        success = core.Run();
    }
    else
    {
        LogFeatureResultSummary(core);
        core.LogError("SandboxApp: Startup sequence failed. Check the runtime log for the last completed step.");
        core.LogError(
            std::string("SandboxApp: Runtime log path = ") +
            core.GetLogFilePath()
        );
    }

    core.Shutdown();
    return success;
}

bool SandboxApp::RunStartupChecks(EngineCore& core)
{
    core.LogInfo("SandboxApp: Running startup checks.");
    core.LogInfo(
        std::string("SandboxApp: Runtime log destination = ") +
        core.GetLogFilePath()
    );

    const unsigned int workerCount = core.GetJobWorkerCount();
    core.LogInfo(
        std::string("SandboxApp: Job workers available = ") +
        std::to_string(workerCount)
    );

    if (workerCount == 0U)
    {
        core.LogWarn("SandboxApp: Job system reported zero workers. Async feature tests will use fallback behavior.");
    }

    SetFeatureDetail(
        std::string("workers = ") +
        std::to_string(workerCount) +
        " | log = " +
        core.GetLogFilePath()
    );

    return true;
}

bool SandboxApp::ExecuteFeatureTest(EngineCore& core, const std::string& name, bool (SandboxApp::*test)(EngineCore&))
{
    m_currentFeatureDetail.clear();
    const bool passed = (this->*test)(core);
    RecordFeatureResult(core, name, passed, m_currentFeatureDetail);
    return passed;
}

void SandboxApp::SetFeatureDetail(const std::string& detail)
{
    m_currentFeatureDetail = detail;
}

void SandboxApp::RecordFeatureResult(
    EngineCore& core,
    const std::string& name,
    bool passed,
    const std::string& detail)
{
    FeatureCheckResult result;
    result.name = name;
    result.passed = passed;
    result.detail = detail;
    m_featureChecks.push_back(result);

    std::string message = std::string("SandboxApp: Feature status | ") + name + " = " + (passed ? "PASS" : "FAIL");
    if (!detail.empty())
    {
        message += " | " + detail;
    }

    if (passed)
    {
        core.LogInfo(message);
    }
    else
    {
        core.LogError(message);
    }
}

std::wstring SandboxApp::BuildFeatureSummaryOverlay() const
{
    if (m_featureChecks.empty())
    {
        return L"STARTUP CHECKS\n  No recorded feature results.";
    }

    std::wstring summary = L"STARTUP CHECKS\n";
    for (const FeatureCheckResult& result : m_featureChecks)
    {
        summary += L"  ";
        summary += std::wstring(result.passed ? L"[PASS] " : L"[FAIL] ");
        summary += std::wstring(result.name.begin(), result.name.end());
        summary += L"\n";

        if (!result.detail.empty())
        {
            summary += L"    ";
            summary += std::wstring(result.detail.begin(), result.detail.end());
            summary += L"\n";
        }
    }

    return summary;
}

void SandboxApp::LogFeatureResultSummary(EngineCore& core) const
{
    std::size_t passedCount = 0;
    for (const FeatureCheckResult& result : m_featureChecks)
    {
        if (result.passed)
        {
            ++passedCount;
        }
    }

    core.LogInfo(
        std::string("SandboxApp: Feature summary | passed = ") +
        std::to_string(passedCount) +
        " | total = " +
        std::to_string(m_featureChecks.size())
    );

    for (const FeatureCheckResult& result : m_featureChecks)
    {
        const std::string message =
            std::string("SandboxApp: Summary item | ") +
            result.name +
            " = " +
            (result.passed ? "PASS" : "FAIL") +
            (result.detail.empty() ? std::string() : std::string(" | ") + result.detail);

        if (result.passed)
        {
            core.LogInfo(message);
        }
        else
        {
            core.LogError(message);
        }
    }
}

bool SandboxApp::RunMemoryFeatureTest(EngineCore& core)
{
    core.LogInfo("SandboxApp: Running memory feature test.");

    TestData* testData = core.NewObject<TestData>(
        MemoryClass::Persistent,
        "TestData",
        42,
        3.14);

    if (!testData)
    {
        core.LogError("SandboxApp: Memory feature test failed to allocate a typed object.");
        SetFeatureDetail("typed object allocation failed");
        return false;
    }

    int* numbers = core.NewArray<int>(
        16,
        MemoryClass::Temporary,
        "TempNumbers");

    if (!numbers)
    {
        core.LogError("SandboxApp: Memory feature test failed to allocate a typed array.");
        core.DeleteObject(testData);
        SetFeatureDetail("typed array allocation failed");
        return false;
    }

    core.LogInfo(
        std::string("SandboxApp: Typed memory after allocate | value = ") +
        std::to_string(testData->value) +
        " | weight = " +
        std::to_string(testData->weight) +
        " | current bytes = " +
        std::to_string(core.GetCurrentMemoryBytes()) +
        " | peak bytes = " +
        std::to_string(core.GetPeakMemoryBytes()) +
        " | allocs = " +
        std::to_string(core.GetMemoryAllocationCount()) +
        " | frees = " +
        std::to_string(core.GetMemoryFreeCount())
    );

    for (int index = 0; index < 16; ++index)
    {
        numbers[index] = index * 2;
    }

    core.DeleteArray(numbers, 16);
    core.DeleteObject(testData);

    core.LogInfo(
        std::string("SandboxApp: Typed memory after release | current bytes = ") +
        std::to_string(core.GetCurrentMemoryBytes()) +
        " | peak bytes = " +
        std::to_string(core.GetPeakMemoryBytes()) +
        " | allocs = " +
        std::to_string(core.GetMemoryAllocationCount()) +
        " | frees = " +
        std::to_string(core.GetMemoryFreeCount())
    );

    SetFeatureDetail(
        std::string("peak bytes = ") +
        std::to_string(core.GetPeakMemoryBytes()) +
        " | allocs = " +
        std::to_string(core.GetMemoryAllocationCount()) +
        " | frees = " +
        std::to_string(core.GetMemoryFreeCount())
    );

    return true;
}

bool SandboxApp::RunRawMemoryFeatureTest(EngineCore& core)
{
    core.LogInfo("SandboxApp: Running raw memory feature test.");

    constexpr std::size_t kBufferSize = 64;
    unsigned char* buffer = static_cast<unsigned char*>(
        core.AllocateMemory(kBufferSize, MemoryClass::Debug, alignof(unsigned char), "RawDebugBuffer"));

    if (!buffer)
    {
        core.LogError("SandboxApp: Raw memory feature test failed to allocate debug buffer.");
        SetFeatureDetail("raw debug buffer allocation failed");
        return false;
    }

    unsigned int checksum = 0;
    for (std::size_t index = 0; index < kBufferSize; ++index)
    {
        buffer[index] = static_cast<unsigned char>((index * 3U) & 0xFFU);
        checksum += buffer[index];
    }

    core.LogInfo(
        std::string("SandboxApp: Raw memory buffer ready | size = ") +
        std::to_string(kBufferSize) +
        " | checksum = " +
        std::to_string(checksum) +
        " | current bytes = " +
        std::to_string(core.GetCurrentMemoryBytes())
    );

    core.FreeMemory(buffer);

    core.LogInfo(
        std::string("SandboxApp: Raw memory buffer released | current bytes = ") +
        std::to_string(core.GetCurrentMemoryBytes()) +
        " | frees = " +
        std::to_string(core.GetMemoryFreeCount())
    );

    SetFeatureDetail(
        std::string("size = ") +
        std::to_string(kBufferSize) +
        " | checksum = " +
        std::to_string(checksum)
    );

    return true;
}

bool SandboxApp::RunFileFeatureTest(EngineCore& core)
{
    core.LogInfo("SandboxApp: Running file feature test.");

    const std::string testDirectory = "runtime_test";
    const std::string textFilePath = testDirectory + "/engine_file_test.txt";
    const std::string binaryFilePath = testDirectory + "/engine_file_test.bin";
    const std::string expectedText = "Engine file module text round-trip.\nLine two stays intact.";
    const std::vector<std::byte> expectedBinary =
    {
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x30},
        std::byte{0x40},
        std::byte{0x5A}
    };

    if (!core.CreateDirectory(testDirectory))
    {
        core.LogError("SandboxApp: File feature test failed to create runtime_test directory.");
        SetFeatureDetail("failed to create runtime_test");
        return false;
    }

    if (!core.WriteTextFile(textFilePath, expectedText))
    {
        core.LogError("SandboxApp: File feature test failed to write text file.");
        SetFeatureDetail("failed to write text file");
        return false;
    }

    std::string loadedText;
    if (!core.ReadTextFile(textFilePath, loadedText))
    {
        core.LogError("SandboxApp: File feature test failed to read text file.");
        SetFeatureDetail("failed to read text file");
        return false;
    }

    if (loadedText != expectedText)
    {
        core.LogError("SandboxApp: File feature test detected text mismatch.");
        SetFeatureDetail("text round-trip mismatch");
        return false;
    }

    if (!core.WriteBinaryFile(binaryFilePath, expectedBinary))
    {
        core.LogError("SandboxApp: File feature test failed to write binary file.");
        SetFeatureDetail("failed to write binary file");
        return false;
    }

    std::vector<std::byte> loadedBinary;
    if (!core.ReadBinaryFile(binaryFilePath, loadedBinary))
    {
        core.LogError("SandboxApp: File feature test failed to read binary file.");
        SetFeatureDetail("failed to read binary file");
        return false;
    }

    if (loadedBinary != expectedBinary)
    {
        core.LogError("SandboxApp: File feature test detected binary mismatch.");
        SetFeatureDetail("binary round-trip mismatch");
        return false;
    }

    core.LogInfo(
        std::string("SandboxApp: File feature test complete | working dir = ") +
        core.GetWorkingDirectory() +
        " | text exists = " +
        (core.FileExists(textFilePath) ? "true" : "false") +
        " | binary size = " +
        std::to_string(core.GetFileSize(binaryFilePath))
    );

    SetFeatureDetail(
        std::string("dir = ") +
        testDirectory +
        " | binary bytes = " +
        std::to_string(core.GetFileSize(binaryFilePath))
    );

    return true;
}

bool SandboxApp::RunGpuFeatureTest(EngineCore& core)
{
    core.LogInfo("SandboxApp: Running GPU compute feature test.");

    std::vector<std::byte> inputData =
    {
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04}
    };
    std::vector<std::byte> outputData(4, std::byte{0x00});

    GpuTaskDesc task;
    task.name = "SandboxAppDummyGpuTask";
    task.input.data = inputData.data();
    task.input.size = inputData.size();
    task.output.data = outputData.data();
    task.output.size = outputData.size();
    task.dispatchX = 1;
    task.dispatchY = 1;
    task.dispatchZ = 1;

    const bool available = core.IsGpuComputeAvailable();
    const std::string backendName = core.GetGpuBackendName();
    const bool submitted = core.SubmitGpuTask(task);
    core.WaitForGpuIdle();

    core.LogInfo(
        std::string("SandboxApp: GPU compute feature test | backend = ") +
        backendName +
        " | available = " +
        (available ? "true" : "false") +
        " | submitted = " +
        (submitted ? "true" : "false")
    );

    if (!available && submitted)
    {
        core.LogError("SandboxApp: GPU compute stub reported unavailable but accepted submission.");
        SetFeatureDetail("stub accepted submission while unavailable");
        return false;
    }

    SetFeatureDetail(
        std::string("backend = ") +
        backendName +
        " | available = " +
        (available ? "true" : "false") +
        " | submitted = " +
        (submitted ? "true" : "false")
    );

    return true;
}

bool SandboxApp::RunJobFeatureTest(EngineCore& core)
{
    core.LogInfo("SandboxApp: Running job feature test.");

    std::atomic<unsigned int> completedJobs = 0;
    std::atomic<int> accumulatedValue = 0;
    constexpr unsigned int kJobCount = 3;

    for (unsigned int index = 0; index < kJobCount; ++index)
    {
        const bool submitted = core.SubmitJob(
            [&, index]()
            {
                int values[4] =
                {
                    static_cast<int>(index),
                    static_cast<int>(index + 1U),
                    static_cast<int>(index + 2U),
                    static_cast<int>(index + 3U)
                };

                accumulatedValue.fetch_add(SumArray(values, 4));
                completedJobs.fetch_add(1U);
                core.LogInfo(
                    std::string("SandboxApp: Feature job completed | index = ") +
                    std::to_string(index) +
                    " | active jobs = " +
                    std::to_string(core.GetActiveJobCount())
                );
            }
        );

        if (!submitted)
        {
            core.LogError(
                std::string("SandboxApp: Failed to submit feature job | index = ") +
                std::to_string(index)
            );
            SetFeatureDetail(
                std::string("job submission failed at index = ") +
                std::to_string(index)
            );
            return false;
        }
    }

    core.WaitForAllJobs();

    core.LogInfo(
        std::string("SandboxApp: Job feature test complete | completed = ") +
        std::to_string(completedJobs.load()) +
        " / " +
        std::to_string(kJobCount) +
        " | accumulated value = " +
        std::to_string(accumulatedValue.load())
    );

    const bool passed = completedJobs.load() == kJobCount;
    SetFeatureDetail(
        std::string("completed = ") +
        std::to_string(completedJobs.load()) +
        "/" +
        std::to_string(kJobCount) +
        " | accumulated = " +
        std::to_string(accumulatedValue.load()) +
        " | peak queued = " +
        std::to_string(core.GetPeakQueuedJobCount())
    );
    return passed;
}

bool SandboxApp::RunInputFeatureTest(EngineCore& core)
{
    core.LogInfo("SandboxApp: Running input feature test.");
    SetFeatureDetail(
        std::string("mouse = (") +
        std::to_string(core.GetMouseX()) +
        ", " +
        std::to_string(core.GetMouseY()) +
        ") | wheel = " +
        std::to_string(core.GetMouseWheelDelta())
    );
    core.LogInfo(
        std::string("SandboxApp: Input ready | mouse = (") +
        std::to_string(core.GetMouseX()) +
        ", " +
        std::to_string(core.GetMouseY()) +
        ") | wheel = " +
        std::to_string(core.GetMouseWheelDelta()) +
        " | left button down = " +
        (core.IsMouseButtonDown(InputModule::MouseButton::Left) ? "true" : "false")
    );
    core.LogInfo("SandboxApp: Move the mouse, scroll the wheel, and click in the window to validate live input state.");
    return true;
}

bool SandboxApp::RunFramePrototypeFeatureTest(EngineCore& core)
{
    core.LogInfo("SandboxApp: Running frame prototype feature test.");

    RenderItemPrototype item;
    item.kind = RenderItemKind::Object3D;

    RenderViewPrototype view;
    view.kind = RenderViewKind::Scene3D;
    view.items.push_back(item);

    RenderPassPrototype pass;
    pass.kind = RenderPassKind::Scene;
    pass.views.push_back(view);

    RenderFramePrototype frame;
    frame.passes.push_back(pass);

    core.SubmitRenderFrame(frame);

    const std::size_t submittedItemCount = core.GetRenderSubmittedItemCount();
    const bool passed = submittedItemCount == 1U;

    core.LogInfo(
        std::string("SandboxApp: Frame prototype feature test | passes = ") +
        std::to_string(frame.passes.size()) +
        " | submitted items = " +
        std::to_string(submittedItemCount)
    );

    if (!passed)
    {
        core.LogError("SandboxApp: Frame prototype feature test expected one submitted item.");
        SetFeatureDetail(
            std::string("submitted items = ") +
            std::to_string(submittedItemCount) +
            " | expected = 1"
        );
        return false;
    }

    SetFeatureDetail(
        std::string("passes = ") +
        std::to_string(frame.passes.size()) +
        " | items = " +
        std::to_string(submittedItemCount)
    );

    return true;
}

bool SandboxApp::RunResourceFeatureTest(EngineCore& core)
{
    core.LogInfo("SandboxApp: Running resource feature test.");

    const std::string resourceDirectory = "runtime_test";
    const std::string resourcePath = resourceDirectory + "/resource_test_texture.bin";
    const std::vector<std::byte> resourceBytes =
    {
        std::byte{0xAA},
        std::byte{0xBB},
        std::byte{0xCC},
        std::byte{0xDD},
        std::byte{0xEE},
        std::byte{0xFF}
    };

    if (!core.CreateDirectory(resourceDirectory))
    {
        core.LogError("SandboxApp: Resource feature test failed to create runtime_test directory.");
        SetFeatureDetail("failed to create resource test directory");
        return false;
    }

    if (!core.WriteBinaryFile(resourcePath, resourceBytes))
    {
        core.LogError("SandboxApp: Resource feature test failed to write resource payload.");
        SetFeatureDetail("failed to write resource payload");
        return false;
    }

    const ResourceHandle validHandle = core.RequestResource(resourcePath, ResourceKind::Texture);
    if (validHandle == 0U)
    {
        core.LogError("SandboxApp: Resource feature test failed to create a valid resource handle.");
        SetFeatureDetail("failed to create valid resource handle");
        return false;
    }

    const ResourceHandle missingHandle = core.RequestResource("runtime_test/missing_texture.bin", ResourceKind::Texture);
    if (missingHandle == 0U)
    {
        core.LogError("SandboxApp: Resource feature test failed to create a missing resource handle.");
        SetFeatureDetail("failed to create missing resource handle");
        return false;
    }

    core.WaitForAllJobs();

    const bool validReady = core.IsResourceReady(validHandle);
    const bool missingReady = core.IsResourceReady(missingHandle);
    const ResourceState validState = core.GetResourceState(validHandle);
    const ResourceState missingState = core.GetResourceState(missingHandle);
    const std::size_t validSize = core.GetResourceSourceByteSize(validHandle);
    const GpuResourceHandle validGpuHandle = core.GetResourceGpuHandle(validHandle);
    const std::size_t validUploadedByteSize = core.GetResourceUploadedByteSize(validHandle);
    const std::string validResolvedPath = core.GetResourceResolvedPath(validHandle);
    const std::string missingError = core.GetResourceLastError(missingHandle);

    core.LogInfo(
        std::string("SandboxApp: Resource feature test | valid state = ") +
        std::to_string(static_cast<int>(validState)) +
        " | valid ready = " +
        (validReady ? "true" : "false") +
        " | valid bytes = " +
        std::to_string(validSize) +
        " | gpu handle = " +
        std::to_string(validGpuHandle) +
        " | uploaded bytes = " +
        std::to_string(validUploadedByteSize) +
        " | valid path = " +
        validResolvedPath
    );

    core.LogInfo(
        std::string("SandboxApp: Resource feature test | missing state = ") +
        std::to_string(static_cast<int>(missingState)) +
        " | missing ready = " +
        (missingReady ? "true" : "false") +
        " | missing error = " +
        missingError
    );

    if (!validReady || validState != ResourceState::Ready)
    {
        core.LogError("SandboxApp: Resource feature test expected the valid resource to reach Ready state.");
        SetFeatureDetail("valid resource did not reach Ready");
        return false;
    }

    if (validSize != resourceBytes.size())
    {
        core.LogError("SandboxApp: Resource feature test detected an unexpected source byte count.");
        SetFeatureDetail("valid resource byte count mismatch");
        return false;
    }

    if (validGpuHandle == 0U || validUploadedByteSize != resourceBytes.size())
    {
        core.LogError("SandboxApp: Resource feature test expected a valid GPU upload handle and uploaded byte count.");
        SetFeatureDetail("gpu upload handle or uploaded byte count mismatch");
        return false;
    }

    if (missingReady || missingState != ResourceState::Failed)
    {
        core.LogError("SandboxApp: Resource feature test expected the missing resource to fail.");
        SetFeatureDetail("missing resource did not fail as expected");
        return false;
    }

    if (missingError.empty())
    {
        core.LogError("SandboxApp: Resource feature test expected a missing-resource error message.");
        SetFeatureDetail("missing resource error text was empty");
        return false;
    }

    SetFeatureDetail(
        std::string("valid bytes = ") +
        std::to_string(validSize) +
        " | uploaded = " +
        std::to_string(validUploadedByteSize) +
        " | gpu handle = " +
        std::to_string(validGpuHandle) +
        " | missing = " +
        missingError
    );

    return true;
}

void SandboxApp::UpdateFrame(EngineCore& core)
{
    UpdateDisplayToggle(core);

    switch (m_displayMode)
    {
    case DisplayMode::TextOverlay:
        RunTextOverlayMode(core);
        break;

    case DisplayMode::PixelRenderer:
        RunPixelRendererMode(core);
        break;

    default:
        break;
    }
}

void SandboxApp::UpdateDisplayToggle(EngineCore& core)
{
    constexpr unsigned char kTabKey = 9;

    if (!core.WasKeyPressed(kTabKey))
    {
        return;
    }

    if (m_displayMode == DisplayMode::TextOverlay)
    {
        m_displayMode = DisplayMode::PixelRenderer;
        core.LogInfo("SandboxApp: Display mode switched to pixel renderer.");
    }
    else
    {
        m_displayMode = DisplayMode::TextOverlay;
        core.LogInfo("SandboxApp: Display mode switched to text overlay.");
    }
}

void SandboxApp::RunTextOverlayMode(EngineCore& core)
{
    core.SetWindowOverlayText(BuildFeatureSummaryOverlay() + L"\n" + core.BuildDebugViewText());
}

void SandboxApp::RunPixelRendererMode(EngineCore& core)
{
    const int width = core.GetFrameWidth();
    const int height = core.GetFrameHeight();
    if (width <= 0 || height <= 0)
    {
        core.SetWindowOverlayText(L"");
        return;
    }

    if (width != m_starFieldWidth || height != m_starFieldHeight || m_stars.empty())
    {
        RebuildStarField(width, height);
    }

    const double deltaTime = core.GetDeltaTime();
    for (BlinkStar& star : m_stars)
    {
        star.blinkTimer += deltaTime;
        if (star.blinkTimer >= star.blinkInterval)
        {
            star.blinkTimer = 0.0;
            star.visible = !star.visible;
        }

        if (star.visible)
        {
            core.PutFramePixel(star.x, star.y, 0x00FFFFFFu);
        }
    }

    core.SetWindowOverlayText(BuildStarFieldOverlay(width, height));
}

void SandboxApp::RebuildStarField(int width, int height)
{
    m_starFieldWidth = width;
    m_starFieldHeight = height;
    m_stars.clear();

    const int starCount = (width * height) / 900;
    const int clampedStarCount = (starCount < 250) ? 250 : ((starCount > 4000) ? 4000 : starCount);

    std::uniform_int_distribution<int> xDistribution(0, width - 1);
    std::uniform_int_distribution<int> yDistribution(0, height - 1);
    std::uniform_real_distribution<double> intervalDistribution(0.08, 1.2);
    std::bernoulli_distribution visibleDistribution(0.7);

    m_stars.reserve(static_cast<std::size_t>(clampedStarCount));
    for (int index = 0; index < clampedStarCount; ++index)
    {
        BlinkStar star;
        star.x = xDistribution(m_rng);
        star.y = yDistribution(m_rng);
        star.visible = visibleDistribution(m_rng);
        star.blinkInterval = intervalDistribution(m_rng);
        star.blinkTimer = intervalDistribution(m_rng) * 0.5;
        m_stars.push_back(star);
    }
}

std::wstring SandboxApp::BuildStarFieldOverlay(int width, int height) const
{
    const int columnCount = std::clamp(width / 18, 24, 72);
    const int rowCount = std::clamp(height / 28, 12, 28);

    std::vector<std::wstring> rows(static_cast<std::size_t>(rowCount), std::wstring(static_cast<std::size_t>(columnCount), L' '));

    for (const BlinkStar& star : m_stars)
    {
        if (!star.visible || width <= 0 || height <= 0)
        {
            continue;
        }

        const int column = std::clamp((star.x * columnCount) / width, 0, columnCount - 1);
        const int row = std::clamp((star.y * rowCount) / height, 0, rowCount - 1);
        rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] = L'*';
    }

    std::wstring overlay = L"STAR FIELD MODE\n";
    overlay += L"  Text fallback for the current placeholder pixel renderer.\n";
    overlay += L"  Tab = return to debug view\n\n";

    for (const std::wstring& row : rows)
    {
        overlay += row;
        overlay += L'\n';
    }

    return overlay;
}

void SandboxApp::LogFeatureSummary(EngineCore& core) const
{
    core.LogInfo("SandboxApp: Feature test harness is ready for expansion.");
    core.LogInfo(
        std::string("SandboxApp: Current diagnostics snapshot | uptime = ") +
        std::to_string(core.GetDiagnosticsUptime()) +
        " | loops = " +
        std::to_string(core.GetDiagnosticsLoopCount()) +
        " | total time = " +
        std::to_string(core.GetTotalTime()) +
        " | queued jobs = " +
        std::to_string(core.GetQueuedJobCount()) +
        " | active jobs = " +
        std::to_string(core.GetActiveJobCount()) +
        " | current memory = " +
        std::to_string(core.GetCurrentMemoryBytes()) +
        " | mouse = (" +
        std::to_string(core.GetMouseX()) +
        ", " +
        std::to_string(core.GetMouseY()) +
        ") | frame = (" +
        std::to_string(core.GetFrameWidth()) +
        ", " +
        std::to_string(core.GetFrameHeight()) +
        ")"
    );

    core.LogInfo("SandboxApp: Runtime controls | Tab = toggle text/pixel display | mouse move/click/scroll = input test | Escape = shutdown.");
}
