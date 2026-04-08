#include "memory_module.hpp"

#include "../../core/engine_context.hpp"

#include <cstdlib>

const char* MemoryModule::GetName() const
{
    return "Memory";
}

bool MemoryModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    ResetTracking();
    m_initialized = true;
    return true;
}

void MemoryModule::Startup(EngineContext& context)
{
    (void)context;

    if (!m_initialized || m_started)
    {
        return;
    }

    m_started = true;
}

void MemoryModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;
}

void MemoryModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto& pair : m_allocations)
        {
            ::operator delete(pair.first, std::align_val_t(pair.second.alignment));
        }

        m_allocations.clear();
    }

    ResetTracking();
    m_started = false;
    m_initialized = false;
}

void* MemoryModule::Allocate(
    std::size_t size,
    MemoryClass memoryClass,
    std::size_t alignment,
    const char* owner)
{
    if (!m_initialized || size == 0)
    {
        return nullptr;
    }

    if (alignment < alignof(void*))
    {
        alignment = alignof(void*);
    }

    void* memory = nullptr;

    try
    {
        memory = ::operator new(size, std::align_val_t(alignment));
    }
    catch (const std::bad_alloc&)
    {
        return nullptr;
    }

    AllocationRecord record;
    record.size = size;
    record.alignment = alignment;
    record.memoryClass = memoryClass;
    record.owner = (owner != nullptr) ? owner : "";

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_allocations[memory] = std::move(record);
        m_stats.currentBytes += size;
        m_stats.totalAllocatedBytes += size;
        m_stats.allocationCount += 1;

        const std::size_t classIndex = static_cast<std::size_t>(memoryClass);
        if (classIndex < m_stats.bytesByClass.size())
        {
            m_stats.bytesByClass[classIndex] += size;

            if (m_stats.bytesByClass[classIndex] > m_stats.peakBytesByClass[classIndex])
            {
                m_stats.peakBytesByClass[classIndex] = m_stats.bytesByClass[classIndex];
            }
        }

        if (m_stats.currentBytes > m_stats.peakBytes)
        {
            m_stats.peakBytes = m_stats.currentBytes;
        }
    }

    return memory;
}

void MemoryModule::Free(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    AllocationRecord record;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_allocations.find(ptr);
        if (it == m_allocations.end())
        {
            return;
        }

        record = it->second;

        if (m_stats.currentBytes >= record.size)
        {
            m_stats.currentBytes -= record.size;
        }
        else
        {
            m_stats.currentBytes = 0;
        }

        const std::size_t classIndex = static_cast<std::size_t>(record.memoryClass);
        if (classIndex < m_stats.bytesByClass.size())
        {
            if (m_stats.bytesByClass[classIndex] >= record.size)
            {
                m_stats.bytesByClass[classIndex] -= record.size;
            }
            else
            {
                m_stats.bytesByClass[classIndex] = 0;
            }
        }

        m_stats.freeCount += 1;
        m_allocations.erase(it);
    }

    ::operator delete(ptr, std::align_val_t(record.alignment));
}

const MemoryStats& MemoryModule::GetStats() const
{
    return m_stats;
}

std::size_t MemoryModule::GetCurrentBytes() const
{
    return m_stats.currentBytes;
}

std::size_t MemoryModule::GetPeakBytes() const
{
    return m_stats.peakBytes;
}

std::size_t MemoryModule::GetAllocationCount() const
{
    return m_stats.allocationCount;
}

std::size_t MemoryModule::GetFreeCount() const
{
    return m_stats.freeCount;
}

std::size_t MemoryModule::GetBytesByClass(MemoryClass memoryClass) const
{
    const std::size_t classIndex = static_cast<std::size_t>(memoryClass);
    if (classIndex >= m_stats.bytesByClass.size())
    {
        return 0;
    }

    return m_stats.bytesByClass[classIndex];
}

std::size_t MemoryModule::GetPeakBytesByClass(MemoryClass memoryClass) const
{
    const std::size_t classIndex = static_cast<std::size_t>(memoryClass);
    if (classIndex >= m_stats.peakBytesByClass.size())
    {
        return 0;
    }

    return m_stats.peakBytesByClass[classIndex];
}

std::string MemoryModule::BuildReport() const
{
    std::ostringstream stream;
    stream
        << "Memory Report"
        << " | current = " << m_stats.currentBytes
        << " | peak = " << m_stats.peakBytes
        << " | total allocated = " << m_stats.totalAllocatedBytes
        << " | allocs = " << m_stats.allocationCount
        << " | frees = " << m_stats.freeCount;

    for (std::size_t index = 0; index < m_stats.bytesByClass.size(); ++index)
    {
        const MemoryClass memoryClass = static_cast<MemoryClass>(index);
        if (memoryClass == MemoryClass::Count)
        {
            continue;
        }

        stream
            << " | "
            << ToString(memoryClass)
            << " = "
            << m_stats.bytesByClass[index]
            << " (peak "
            << m_stats.peakBytesByClass[index]
            << ")";
    }

    return stream.str();
}

bool MemoryModule::IsInitialized() const
{
    return m_initialized;
}

bool MemoryModule::IsStarted() const
{
    return m_started;
}

const char* MemoryModule::ToString(MemoryClass memoryClass)
{
    return GetMemoryClassName(memoryClass);
}

void MemoryModule::ResetTracking()
{
    m_stats = {};
} 
