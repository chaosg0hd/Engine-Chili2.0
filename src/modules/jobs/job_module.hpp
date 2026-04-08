#pragma once

#include "../../core/module.hpp"
#include "ijob_service.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class EngineContext;

class JobModule : public IModule, public IJobService
{
public:
    JobModule();
    ~JobModule() override;

    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    void Submit(JobFunction job) override;
    void WaitIdle() override;

    unsigned int GetWorkerCount() const override;
    unsigned int GetIdleWorkerCount() const override;
    std::size_t GetQueuedJobCount() const override;
    std::size_t GetPeakQueuedJobCount() const override;
    unsigned int GetActiveJobCount() const override;
    bool HasPendingJobs() const override;
    bool IsIdle() const override;
    unsigned long long GetSubmittedJobCount() const override;
    unsigned long long GetCompletedJobCount() const override;

    bool IsInitialized() const;
    bool IsStarted() const override;

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
    std::atomic<std::size_t> m_peakQueuedJobs = 0;
    std::atomic<unsigned long long> m_submittedJobCount = 0;
    std::atomic<unsigned long long> m_completedJobCount = 0;
};
