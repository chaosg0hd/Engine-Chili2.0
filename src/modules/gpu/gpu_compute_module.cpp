#include "gpu_compute_module.hpp"

#include "../../core/engine_context.hpp"

const char* GpuComputeModule::GetName() const
{
    return "GpuCompute";
}

bool GpuComputeModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    m_gpuComputeAvailable = false;
    m_backendName = "Stub";
    m_lastTaskName.clear();
    m_initialized = true;
    return true;
}

void GpuComputeModule::Startup(EngineContext& context)
{
    (void)context;

    if (!m_initialized || m_started)
    {
        return;
    }

    m_started = true;
}

void GpuComputeModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;
}

void GpuComputeModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    m_lastTaskName.clear();
    m_started = false;
    m_initialized = false;
}

bool GpuComputeModule::IsGpuComputeAvailable() const
{
    return m_gpuComputeAvailable;
}

bool GpuComputeModule::SubmitGpuTask(const GpuTaskDesc& task)
{
    if (!m_started || !ValidateTask(task))
    {
        return false;
    }

    m_lastTaskName = task.name;

    if (!m_gpuComputeAvailable)
    {
        return false;
    }

    return true;
}

void GpuComputeModule::WaitForGpuIdle()
{
    // Stub backend: no queued GPU work to wait on yet.
}

const char* GpuComputeModule::GetBackendName() const
{
    return m_backendName.c_str();
}

bool GpuComputeModule::IsInitialized() const
{
    return m_initialized;
}

bool GpuComputeModule::IsStarted() const
{
    return m_started;
}

bool GpuComputeModule::ValidateTask(const GpuTaskDesc& task) const
{
    if (task.name.empty())
    {
        return false;
    }

    if (task.dispatchX == 0 || task.dispatchY == 0 || task.dispatchZ == 0)
    {
        return false;
    }

    if ((task.input.size > 0) && (task.input.data == nullptr))
    {
        return false;
    }

    if ((task.output.size > 0) && (task.output.data == nullptr))
    {
        return false;
    }

    return true;
}
