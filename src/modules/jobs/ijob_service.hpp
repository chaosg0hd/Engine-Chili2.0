#pragma once

#include <cstddef>
#include <functional>

class IJobService
{
public:
    using JobFunction = std::function<void()>;

    virtual ~IJobService() = default;

    virtual void Submit(JobFunction job) = 0;
    virtual void WaitIdle() = 0;

    virtual unsigned int GetWorkerCount() const = 0;
    virtual unsigned int GetIdleWorkerCount() const = 0;
    virtual std::size_t GetQueuedJobCount() const = 0;
    virtual std::size_t GetPeakQueuedJobCount() const = 0;
    virtual unsigned int GetActiveJobCount() const = 0;
    virtual bool HasPendingJobs() const = 0;
    virtual bool IsIdle() const = 0;
    virtual unsigned long long GetSubmittedJobCount() const = 0;
    virtual unsigned long long GetCompletedJobCount() const = 0;
    virtual bool IsStarted() const = 0;
};
