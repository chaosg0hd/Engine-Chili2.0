#include "app.hpp"

#include "../core/engine_core.hpp"

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

bool App::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    bool success = RunStartupChecks(core);

    if (success)
    {
        success = RunMemoryFeatureTest(core);
    }

    if (success)
    {
        success = RunRawMemoryFeatureTest(core);
    }

    if (success)
    {
        success = RunFileFeatureTest(core);
    }

    if (success)
    {
        success = RunGpuFeatureTest(core);
    }

    if (success)
    {
        success = RunJobFeatureTest(core);
    }

    if (success)
    {
        success = RunInputFeatureTest(core);
    }

    if (success)
    {
        LogFeatureSummary(core);
        core.SetFrameCallback(
            [this](EngineCore& callbackCore)
            {
                UpdateFrame(callbackCore);
            });
        core.LogInfo("App: Entering runtime loop. Close the window or press Escape to exit.");
        success = core.Run();
    }

    core.Shutdown();
    return success;
}

bool App::RunStartupChecks(EngineCore& core)
{
    core.LogInfo("App: Running startup checks.");

    const unsigned int workerCount = core.GetJobWorkerCount();
    core.LogInfo(
        std::string("App: Job workers available = ") +
        std::to_string(workerCount)
    );

    if (workerCount == 0U)
    {
        core.LogWarn("App: Job system reported zero workers. Async feature tests will use fallback behavior.");
    }

    return true;
}

bool App::RunMemoryFeatureTest(EngineCore& core)
{
    core.LogInfo("App: Running memory feature test.");

    TestData* testData = core.NewObject<TestData>(
        MemoryClass::Persistent,
        "TestData",
        42,
        3.14);

    if (!testData)
    {
        core.LogError("App: Memory feature test failed to allocate a typed object.");
        return false;
    }

    int* numbers = core.NewArray<int>(
        16,
        MemoryClass::Temporary,
        "TempNumbers");

    if (!numbers)
    {
        core.LogError("App: Memory feature test failed to allocate a typed array.");
        core.DeleteObject(testData);
        return false;
    }

    core.LogInfo(
        std::string("App: Typed memory after allocate | value = ") +
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
        std::string("App: Typed memory after release | current bytes = ") +
        std::to_string(core.GetCurrentMemoryBytes()) +
        " | peak bytes = " +
        std::to_string(core.GetPeakMemoryBytes()) +
        " | allocs = " +
        std::to_string(core.GetMemoryAllocationCount()) +
        " | frees = " +
        std::to_string(core.GetMemoryFreeCount())
    );

    return true;
}

bool App::RunRawMemoryFeatureTest(EngineCore& core)
{
    core.LogInfo("App: Running raw memory feature test.");

    constexpr std::size_t kBufferSize = 64;
    unsigned char* buffer = static_cast<unsigned char*>(
        core.AllocateMemory(kBufferSize, MemoryClass::Debug, alignof(unsigned char), "RawDebugBuffer"));

    if (!buffer)
    {
        core.LogError("App: Raw memory feature test failed to allocate debug buffer.");
        return false;
    }

    unsigned int checksum = 0;
    for (std::size_t index = 0; index < kBufferSize; ++index)
    {
        buffer[index] = static_cast<unsigned char>((index * 3U) & 0xFFU);
        checksum += buffer[index];
    }

    core.LogInfo(
        std::string("App: Raw memory buffer ready | size = ") +
        std::to_string(kBufferSize) +
        " | checksum = " +
        std::to_string(checksum) +
        " | current bytes = " +
        std::to_string(core.GetCurrentMemoryBytes())
    );

    core.FreeMemory(buffer);

    core.LogInfo(
        std::string("App: Raw memory buffer released | current bytes = ") +
        std::to_string(core.GetCurrentMemoryBytes()) +
        " | frees = " +
        std::to_string(core.GetMemoryFreeCount())
    );

    return true;
}

bool App::RunFileFeatureTest(EngineCore& core)
{
    core.LogInfo("App: Running file feature test.");

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
        core.LogError("App: File feature test failed to create runtime_test directory.");
        return false;
    }

    if (!core.WriteTextFile(textFilePath, expectedText))
    {
        core.LogError("App: File feature test failed to write text file.");
        return false;
    }

    std::string loadedText;
    if (!core.ReadTextFile(textFilePath, loadedText))
    {
        core.LogError("App: File feature test failed to read text file.");
        return false;
    }

    if (loadedText != expectedText)
    {
        core.LogError("App: File feature test detected text mismatch.");
        return false;
    }

    if (!core.WriteBinaryFile(binaryFilePath, expectedBinary))
    {
        core.LogError("App: File feature test failed to write binary file.");
        return false;
    }

    std::vector<std::byte> loadedBinary;
    if (!core.ReadBinaryFile(binaryFilePath, loadedBinary))
    {
        core.LogError("App: File feature test failed to read binary file.");
        return false;
    }

    if (loadedBinary != expectedBinary)
    {
        core.LogError("App: File feature test detected binary mismatch.");
        return false;
    }

    core.LogInfo(
        std::string("App: File feature test complete | working dir = ") +
        core.GetWorkingDirectory() +
        " | text exists = " +
        (core.FileExists(textFilePath) ? "true" : "false") +
        " | binary size = " +
        std::to_string(core.GetFileSize(binaryFilePath))
    );

    return true;
}

bool App::RunGpuFeatureTest(EngineCore& core)
{
    core.LogInfo("App: Running GPU compute feature test.");

    std::vector<std::byte> inputData =
    {
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04}
    };
    std::vector<std::byte> outputData(4, std::byte{0x00});

    GpuTaskDesc task;
    task.name = "AppDummyGpuTask";
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
        std::string("App: GPU compute feature test | backend = ") +
        backendName +
        " | available = " +
        (available ? "true" : "false") +
        " | submitted = " +
        (submitted ? "true" : "false")
    );

    if (!available && submitted)
    {
        core.LogError("App: GPU compute stub reported unavailable but accepted submission.");
        return false;
    }

    return true;
}

bool App::RunJobFeatureTest(EngineCore& core)
{
    core.LogInfo("App: Running job feature test.");

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
                    std::string("App: Feature job completed | index = ") +
                    std::to_string(index) +
                    " | active jobs = " +
                    std::to_string(core.GetActiveJobCount())
                );
            }
        );

        if (!submitted)
        {
            core.LogError(
                std::string("App: Failed to submit feature job | index = ") +
                std::to_string(index)
            );
            return false;
        }
    }

    core.WaitForAllJobs();

    core.LogInfo(
        std::string("App: Job feature test complete | completed = ") +
        std::to_string(completedJobs.load()) +
        " / " +
        std::to_string(kJobCount) +
        " | accumulated value = " +
        std::to_string(accumulatedValue.load())
    );

    return completedJobs.load() == kJobCount;
}

bool App::RunInputFeatureTest(EngineCore& core)
{
    core.LogInfo("App: Running input feature test.");
    core.LogInfo(
        std::string("App: Input ready | mouse = (") +
        std::to_string(core.GetMouseX()) +
        ", " +
        std::to_string(core.GetMouseY()) +
        ") | wheel = " +
        std::to_string(core.GetMouseWheelDelta()) +
        " | left button down = " +
        (core.IsMouseButtonDown(InputModule::MouseButton::Left) ? "true" : "false")
    );
    core.LogInfo("App: Move the mouse, scroll the wheel, and click in the window to validate live input state.");
    return true;
}

void App::UpdateFrame(EngineCore& core)
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

void App::UpdateDisplayToggle(EngineCore& core)
{
    constexpr unsigned char kTabKey = 9;

    if (!core.WasKeyPressed(kTabKey))
    {
        return;
    }

    if (m_displayMode == DisplayMode::TextOverlay)
    {
        m_displayMode = DisplayMode::PixelRenderer;
        core.LogInfo("App: Display mode switched to pixel renderer.");
    }
    else
    {
        m_displayMode = DisplayMode::TextOverlay;
        core.LogInfo("App: Display mode switched to text overlay.");
    }
}

void App::RunTextOverlayMode(EngineCore& core)
{
    core.ClearFrame(0x00000000u);
    core.PresentFrame();

    const double deltaTime = core.GetDeltaTime();
    const double framesPerSecond = (deltaTime > 0.0) ? (1.0 / deltaTime) : 0.0;
    const std::string settingsPath = core.GetSettingsPath();
    const std::string workingDirectory = core.GetWorkingDirectory();
    const std::string gpuBackendName = core.GetGpuBackendName();
    const std::wstring leftMouseState = core.IsMouseButtonDown(InputModule::MouseButton::Left) ? L"Down" : L"Up";
    const std::wstring rightMouseState = core.IsMouseButtonDown(InputModule::MouseButton::Right) ? L"Down" : L"Up";
    const std::wstring middleMouseState = core.IsMouseButtonDown(InputModule::MouseButton::Middle) ? L"Down" : L"Up";
    const std::wstring settingsPathText(settingsPath.begin(), settingsPath.end());
    const std::wstring workingDirectoryText(workingDirectory.begin(), workingDirectory.end());
    const std::wstring gpuBackendText(gpuBackendName.begin(), gpuBackendName.end());

    core.SetWindowOverlayText(
        L"DISPLAY MODE: TEXT\n"
        L"Runtime\n"
        L"  Uptime: " + std::to_wstring(core.GetTotalTime()) + L"s\n"
        L"  Delta Time: " + std::to_wstring(deltaTime) + L"s\n"
        L"  FPS: " + std::to_wstring(framesPerSecond) + L"\n"
        L"  FPS Limit: " + std::to_wstring(core.GetFpsLimit()) + L"\n"
        L"  Loops: " + std::to_wstring(core.GetDiagnosticsLoopCount()) + L"\n"
        L"\n"
        L"Render\n"
        L"  Frame: " + std::to_wstring(core.GetFrameWidth()) + L" x " + std::to_wstring(core.GetFrameHeight()) + L"\n"
        L"  Mode: Text Overlay\n"
        L"\n"
        L"Input\n"
        L"  Mouse: (" + std::to_wstring(core.GetMouseX()) + L", " + std::to_wstring(core.GetMouseY()) + L")\n"
        L"  Mouse Delta: (" + std::to_wstring(core.GetMouseDeltaX()) + L", " + std::to_wstring(core.GetMouseDeltaY()) + L")\n"
        L"  Wheel: " + std::to_wstring(core.GetMouseWheelDelta()) + L"\n"
        L"  LMB/RMB/MMB: " + leftMouseState + L" / " + rightMouseState + L" / " + middleMouseState + L"\n"
        L"\n"
        L"Jobs\n"
        L"  Workers: " + std::to_wstring(core.GetJobWorkerCount()) + L"\n"
        L"  Queued: " + std::to_wstring(core.GetQueuedJobCount()) + L"\n"
        L"  Active: " + std::to_wstring(core.GetActiveJobCount()) + L"\n"
        L"\n"
        L"Memory\n"
        L"  Current Bytes: " + std::to_wstring(core.GetCurrentMemoryBytes()) + L"\n"
        L"  Peak Bytes: " + std::to_wstring(core.GetPeakMemoryBytes()) + L"\n"
        L"  Allocations: " + std::to_wstring(core.GetMemoryAllocationCount()) + L"\n"
        L"  Frees: " + std::to_wstring(core.GetMemoryFreeCount()) + L"\n"
        L"\n"
        L"System\n"
        L"  Settings: " + settingsPathText + L"\n"
        L"  Working Dir: " + workingDirectoryText + L"\n"
        L"  GPU Backend: " + gpuBackendText + L"\n"
        L"  GPU Available: " + std::wstring(core.IsGpuComputeAvailable() ? L"Yes" : L"No") + L"\n"
        L"\n"
        L"Controls\n"
        L"  Tab = Toggle Renderer\n"
        L"  Escape = Exit");
}

void App::RunPixelRendererMode(EngineCore& core)
{
    core.ClearFrame(0x00000000u);

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

    core.PresentFrame();
    core.SetWindowOverlayText(L"");
}

void App::RebuildStarField(int width, int height)
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

void App::LogFeatureSummary(EngineCore& core) const
{
    core.LogInfo("App: Feature test harness is ready for expansion.");
    core.LogInfo(
        std::string("App: Current diagnostics snapshot | uptime = ") +
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

    core.LogInfo("App: Runtime controls | Tab = toggle text/pixel display | mouse move/click/scroll = input test | Escape = shutdown.");
}
