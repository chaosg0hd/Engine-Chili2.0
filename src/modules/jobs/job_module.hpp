#pragma once

#include "../../core/module.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class EngineContext;

class JobModule : public IModule
{
public:
    using JobFunction = std::function<void()>;

public:
    JobModule();
    ~JobModule() override;

    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    void Submit(JobFunction job);
    void WaitIdle();

    unsigned int GetWorkerCount() const;
    std::size_t GetQueuedJobCount() const;
    unsigned int GetActiveJobCount() const;
    bool HasPendingJobs() const;
    bool IsIdle() const;
    unsigned long long GetSubmittedJobCount() const;
    unsigned long long GetCompletedJobCount() const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    void StartWorkers();
    void StopWorkers();
    void WorkerLoop();
    unsigned int DetermineWorkerCount() const;

private:
    bool m_initialized = false;
    bool m_started = false;

    unsigned int m_workerCount = 0;

    std::vector<std::thread> m_workers;
    std::queue<JobFunction> m_jobs;

    mutable std::mutex m_queueMutex;
    std::condition_variable m_jobAvailableCondition;
    std::condition_variable m_idleCondition;

    std::atomic<bool> m_stopRequested = false;
    std::atomic<unsigned int> m_activeJobs = 0;
    std::atomic<unsigned long long> m_submittedJobCount = 0;
    std::atomic<unsigned long long> m_completedJobCount = 0;
};
