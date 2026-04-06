#include "app.hpp"

#include "../core/engine_core.hpp"

#include <atomic>
#include <cstddef>
#include <string>

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
        success = RunJobFeatureTest(core);
    }

    if (success)
    {
        success = RunInputFeatureTest(core);
    }

    if (success)
    {
        LogFeatureSummary(core);
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

    core.LogInfo("App: Runtime controls | mouse move/click/scroll = input test | Escape = shutdown.");
}
