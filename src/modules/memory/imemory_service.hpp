#pragma once

#include "memory_types.hpp"

#include <cstddef>
#include <string>

class IMemoryService
{
public:
    virtual ~IMemoryService() = default;

    virtual void* Allocate(
        std::size_t size,
        MemoryClass memoryClass,
        std::size_t alignment = alignof(std::max_align_t),
        const char* owner = nullptr) = 0;

    virtual void Free(void* ptr) = 0;
    virtual const MemoryStats& GetStats() const = 0;
    virtual std::size_t GetCurrentBytes() const = 0;
    virtual std::size_t GetPeakBytes() const = 0;
    virtual std::size_t GetAllocationCount() const = 0;
    virtual std::size_t GetFreeCount() const = 0;
    virtual std::size_t GetBytesByClass(MemoryClass memoryClass) const = 0;
    virtual std::size_t GetPeakBytesByClass(MemoryClass memoryClass) const = 0;
    virtual std::string BuildReport() const = 0;
    virtual bool IsStarted() const = 0;
};
