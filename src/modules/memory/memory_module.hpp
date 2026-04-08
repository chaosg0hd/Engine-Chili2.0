#pragma once

#include "../../core/module.hpp"
#include "imemory_service.hpp"
#include "memory_types.hpp"

#include <cstddef>
#include <mutex>
#include <new>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

class EngineContext;

struct AllocationRecord
{
    std::size_t size = 0;
    std::size_t alignment = 0;
    MemoryClass memoryClass = MemoryClass::Unknown;
    std::string owner;
};

class MemoryModule : public IModule, public IMemoryService
{
public:
    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    void* Allocate(
        std::size_t size,
        MemoryClass memoryClass,
        std::size_t alignment = alignof(std::max_align_t),
        const char* owner = nullptr) override;

    void Free(void* ptr) override;

    template<typename T, typename... Args>
    T* New(
        MemoryClass memoryClass,
        const char* owner,
        Args&&... args)
    {
        void* memory = Allocate(sizeof(T), memoryClass, alignof(T), owner);
        if (!memory)
        {
            return nullptr;
        }

        return new (memory) T(std::forward<Args>(args)...);
    }

    template<typename T>
    void Delete(T* ptr)
    {
        if (!ptr)
        {
            return;
        }

        ptr->~T();
        Free(ptr);
    }

    template<typename T>
    T* NewArray(
        std::size_t count,
        MemoryClass memoryClass,
        const char* owner = nullptr)
    {
        if (count == 0)
        {
            return nullptr;
        }

        void* memory = Allocate(sizeof(T) * count, memoryClass, alignof(T), owner);
        if (!memory)
        {
            return nullptr;
        }

        T* typed = static_cast<T*>(memory);
        for (std::size_t i = 0; i < count; ++i)
        {
            new (&typed[i]) T();
        }

        return typed;
    }

    template<typename T>
    void DeleteArray(T* ptr, std::size_t count)
    {
        if (!ptr)
        {
            return;
        }

        for (std::size_t i = 0; i < count; ++i)
        {
            ptr[i].~T();
        }

        Free(ptr);
    }

    const MemoryStats& GetStats() const override;

    std::size_t GetCurrentBytes() const override;
    std::size_t GetPeakBytes() const override;
    std::size_t GetAllocationCount() const override;
    std::size_t GetFreeCount() const override;
    std::size_t GetBytesByClass(MemoryClass memoryClass) const override;
    std::size_t GetPeakBytesByClass(MemoryClass memoryClass) const override;
    std::string BuildReport() const override;

    bool IsInitialized() const;
    bool IsStarted() const override;

    static const char* ToString(MemoryClass memoryClass);

private:
    void ResetTracking();

private:
    bool m_initialized = false;
    bool m_started = false;

    MemoryStats m_stats;
    std::unordered_map<void*, AllocationRecord> m_allocations;
    mutable std::mutex m_mutex;
};
