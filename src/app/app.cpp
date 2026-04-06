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
                RunPixelBlinkTest(callbackCore);
                UpdateRollGame(callbackCore);
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

void App::RunPixelBlinkTest(EngineCore& core)
{
    constexpr unsigned char kTabKey = 9;

    if (core.WasKeyPressed(kTabKey))
    {
        m_pixelTestEnabled = !m_pixelTestEnabled;
        core.LogInfo(std::string("App: Pixel blink test ") + (m_pixelTestEnabled ? "enabled." : "disabled."));
    }

    core.ClearFrame(0x00000000u);

    if (!m_pixelTestEnabled)
    {
        core.PresentFrame();
        return;
    }

    const int width = core.GetFrameWidth();
    const int height = core.GetFrameHeight();
    if (width <= 0 || height <= 0)
    {
        return;
    }

    std::uniform_int_distribution<int> xDistribution(0, width - 1);
    std::uniform_int_distribution<int> yDistribution(0, height - 1);

    constexpr int kPixelCount = 2000;
    for (int index = 0; index < kPixelCount; ++index)
    {
        const int x = xDistribution(m_rng);
        const int y = yDistribution(m_rng);
        core.PutFramePixel(x, y, 0x00FFFFFFu);
    }

    core.PresentFrame();
}

void App::UpdateRollGame(EngineCore& core)
{
    constexpr double kRollIntervalSeconds = 0.05;
    constexpr int kRollsPerTick = 1000;

    if (core.WasKeyPressed('P'))
    {
        m_paused = !m_paused;
        core.LogInfo(std::string("App: Roll stress test ") + (m_paused ? "paused." : "resumed."));
    }

    if (core.WasKeyPressed('R'))
    {
        m_score = 0;
        m_lastRoll = 0;
        m_totalRolls = 0;
        m_tenHits = 0;
        m_rollTimer = 0.0;
        core.LogInfo("App: Roll stress test state reset.");
    }

    m_rollTimer += core.GetDeltaTime();

    if (!m_paused)
    {
        std::uniform_int_distribution<int> distribution(1, 10);

        while (m_rollTimer >= kRollIntervalSeconds)
        {
            m_rollTimer -= kRollIntervalSeconds;

            for (int rollIndex = 0; rollIndex < kRollsPerTick; ++rollIndex)
            {
                const int roll = distribution(m_rng);
                m_lastRoll = roll;
                ++m_totalRolls;

                if (roll == 10)
                {
                    ++m_tenHits;
                    m_score += 100;
                }
                else
                {
                    m_score -= 1;
                }
            }
        }
    }

    core.SetWindowOverlayText(
        std::wstring(L"ROLL STRESS TEST\n") +
        L"Pixel Test: " + std::wstring(m_pixelTestEnabled ? L"ON" : L"OFF") + L"\n" +
        L"Last Roll: " + std::to_wstring(m_lastRoll) + L"\n"
        L"Score: " + std::to_wstring(m_score) + L"\n"
        L"Total Rolls: " + std::to_wstring(m_totalRolls) + L"\n"
        L"Critical 10s: " + std::to_wstring(m_tenHits) + L"\n"
        L"Rolls / Tick: " + std::to_wstring(kRollsPerTick) + L"\n"
        L"Roll Interval: " + std::to_wstring(kRollIntervalSeconds) + L"\n"
        L"Resolution: " + std::to_wstring(core.GetFrameWidth()) + L" x " + std::to_wstring(core.GetFrameHeight()) + L"\n"
        L"State: " + std::wstring(m_paused ? L"Paused" : L"Running") + L"\n"
        L"Tab = Pixel Toggle | P = Pause/Resume | R = Reset");
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
        ")"
    );

    core.LogInfo("App: Runtime controls | Tab = pixel toggle | mouse move/click/scroll = input test | P = pause roll test | R = reset | Escape = shutdown.");
}
