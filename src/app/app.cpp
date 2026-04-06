#include "app.hpp"

#include "../core/engine_core.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include <chrono>

namespace
{
    bool IsPrime(std::uint64_t value)
    {
        if (value < 2ULL)
        {
            return false;
        }

        if (value == 2ULL)
        {
            return true;
        }

        if ((value % 2ULL) == 0ULL)
        {
            return false;
        }

        for (std::uint64_t divisor = 3ULL; divisor <= (value / divisor); divisor += 2ULL)
        {
            if ((value % divisor) == 0ULL)
            {
                return false;
            }
        }

        return true;
    }
}

bool App::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    core.LogInfo("App: Starting continuous prime discovery test.");

    const unsigned int workerCount = core.GetJobWorkerCount();
    const unsigned int dispatchCount = (workerCount > 0U) ? workerCount : 1U;

    core.LogInfo(
        std::string("App: Job workers available = ") +
        std::to_string(workerCount)
    );

    std::atomic<bool> stopRequested = false;
    std::atomic<std::uint64_t> totalCandidatesChecked = 0;
    std::atomic<std::uint64_t> uniquePrimeCount = 0;
    std::atomic<std::uint64_t> latestPrime = 0;

    std::unordered_set<std::uint64_t> uniquePrimes;
    uniquePrimes.reserve(100000);

    std::mutex uniquePrimeMutex;

    for (unsigned int workerIndex = 0; workerIndex < dispatchCount; ++workerIndex)
    {
        const bool submitted = core.SubmitJob(
            [&, workerIndex]()
            {
                std::random_device rd;
                std::mt19937_64 rng(
                    rd() ^
                    (static_cast<std::uint64_t>(workerIndex + 1U) * 0x9E3779B97F4A7C15ULL)
                );

                std::uniform_int_distribution<std::uint64_t> dist(
                    1000000001ULL,
                    4000000001ULL
                );

                while (!stopRequested.load())
                {
                    std::uint64_t candidate = dist(rng);

                    if ((candidate % 2ULL) == 0ULL)
                    {
                        ++candidate;
                    }

                    totalCandidatesChecked.fetch_add(1ULL);

                    if (!IsPrime(candidate))
                    {
                        continue;
                    }

                    {
                        std::lock_guard<std::mutex> lock(uniquePrimeMutex);

                        const auto insertResult = uniquePrimes.insert(candidate);
                        if (insertResult.second)
                        {
                            latestPrime.store(candidate);
                            uniquePrimeCount.fetch_add(1ULL);
                        }
                    }
                }
            }
        );

        if (!submitted)
        {
            core.LogWarn(
                std::string("App: Failed to submit prime hunter job at worker index ") +
                std::to_string(workerIndex)
            );
        }
    }

    core.LogInfo(
        std::string("App: Continuous prime jobs submitted = ") +
        std::to_string(dispatchCount)
    );

    std::thread monitorThread(
        [&]()
        {
            std::uint64_t lastReportedPrime = 0;

            while (!stopRequested.load())
            {
                const std::uint64_t currentPrime = latestPrime.load();
                const std::uint64_t currentUniqueCount = uniquePrimeCount.load();
                const std::uint64_t currentCheckedCount = totalCandidatesChecked.load();

                if (currentPrime != 0ULL && currentPrime != lastReportedPrime)
                {
                    core.LogInfo(
                        std::string("Prime Update | latest = ") +
                        std::to_string(currentPrime) +
                        " | unique count = " +
                        std::to_string(currentUniqueCount) +
                        " | checked = " +
                        std::to_string(currentCheckedCount) +
                        " | active jobs = " +
                        std::to_string(core.GetActiveJobCount())
                    );

                    lastReportedPrime = currentPrime;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    );

    const bool result = core.Run();

    stopRequested.store(true);
    core.WaitForAllJobs();

    if (monitorThread.joinable())
    {
        monitorThread.join();
    }

    core.LogInfo(
        std::string("App: Prime test stopped | unique primes = ") +
        std::to_string(uniquePrimeCount.load()) +
        " | checked = " +
        std::to_string(totalCandidatesChecked.load()) +
        " | latest prime = " +
        std::to_string(latestPrime.load())
    );

    core.Shutdown();
    return result;
}