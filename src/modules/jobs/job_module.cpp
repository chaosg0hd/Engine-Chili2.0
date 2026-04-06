#include "job_module.hpp"

#include "../../core/engine_context.hpp"

#include <utility>

JobModule::JobModule()
    : m_initialized(false),
      m_started(false),
      m_workerCount(0),
      m_stopRequested(false),
      m_activeJobs(0)
{
}

JobModule::~JobModule()
{
    if (m_started || m_initialized)
    {
        m_stopRequested.store(true);
        m_jobAvailableCondition.notify_all();

        for (std::thread& worker : m_workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }
}

const char* JobModule::GetName() const
{
    return "Jobs";
}

bool JobModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    m_workerCount = DetermineWorkerCount();
    m_initialized = true;
    return true;
}

void JobModule::Startup(EngineContext& context)
{
    (void)context;

    if (!m_initialized || m_started)
    {
        return;
    }

    StartWorkers();
    m_started = true;
}

void JobModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;

    if (!m_started)
    {
        return;
    }

    // Placeholder for future:
    // - queue pressure metrics
    // - starvation detection
    // - worker utilization metrics
    // - scheduling diagnostics
}

void JobModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    StopWorkers();

    m_started = false;
    m_initialized = false;
    m_workerCount = 0;
}

void JobModule::Submit(JobFunction job)
{
    if (!m_started || !job)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_jobs.push(std::move(job));
    }

    m_jobAvailableCondition.notify_one();
}

void JobModule::WaitIdle()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    m_idleCondition.wait(lock, [this]()
    {
        return m_jobs.empty() && (m_activeJobs.load() == 0);
    });
}

unsigned int JobModule::GetWorkerCount() const
{
    return m_workerCount;
}

std::size_t JobModule::GetQueuedJobCount() const
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_jobs.size();
}

unsigned int JobModule::GetActiveJobCount() const
{
    return m_activeJobs.load();
}

bool JobModule::IsInitialized() const
{
    return m_initialized;
}

bool JobModule::IsStarted() const
{
    return m_started;
}

void JobModule::StartWorkers()
{
    m_stopRequested.store(false);

    for (unsigned int i = 0; i < m_workerCount; ++i)
    {
        m_workers.emplace_back(&JobModule::WorkerLoop, this);
    }
}

void JobModule::StopWorkers()
{
    WaitIdle();

    m_stopRequested.store(true);
    m_jobAvailableCondition.notify_all();

    for (std::thread& worker : m_workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    m_workers.clear();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::queue<JobFunction> emptyQueue;
        m_jobs.swap(emptyQueue);
    }

    m_activeJobs.store(0);
}

void JobModule::WorkerLoop()
{
    while (true)
    {
        JobFunction job;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            m_jobAvailableCondition.wait(lock, [this]()
            {
                return m_stopRequested.load() || !m_jobs.empty();
            });

            if (m_stopRequested.load() && m_jobs.empty())
            {
                return;
            }

            job = std::move(m_jobs.front());
            m_jobs.pop();
            m_activeJobs.fetch_add(1);
        }

        job();

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);

            m_activeJobs.fetch_sub(1);

            if (m_jobs.empty() && (m_activeJobs.load() == 0))
            {
                m_idleCondition.notify_all();
            }
        }
    }
}

unsigned int JobModule::DetermineWorkerCount() const
{
    const unsigned int hardwareThreads = std::thread::hardware_concurrency();

    if (hardwareThreads <= 1)
    {
        return 1;
    }

    unsigned int workerCount = hardwareThreads - 1;

    if (workerCount > 8)
    {
        workerCount = 8;
    }

    if (workerCount == 0)
    {
        workerCount = 1;
    }

    return workerCount;
}