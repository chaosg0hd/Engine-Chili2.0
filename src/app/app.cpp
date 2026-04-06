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
        success = RunJobFeatureTest(core);
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

bool App::RunJobFeatureTest(EngineCore& core)
{
    core.LogInfo("App: Running job feature test.");

    std::atomic<unsigned int> completedJobs = 0;
    constexpr unsigned int kJobCount = 3;

    for (unsigned int index = 0; index < kJobCount; ++index)
    {
        const bool submitted = core.SubmitJob(
            [&, index]()
            {
                completedJobs.fetch_add(1U);
                core.LogInfo(
                    std::string("App: Feature job completed | index = ") +
                    std::to_string(index)
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
        std::to_string(kJobCount)
    );

    return completedJobs.load() == kJobCount;
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
        std::to_string(core.GetTotalTime())
    );
}
