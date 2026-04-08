#pragma once

#include <array>
#include <cstddef>
#include <string>

enum class MemoryClass : std::size_t
{
    Unknown = 0,
    Core,
    Module,
    Frame,
    Resource,
    Persistent,
    Temporary,
    Job,
    Debug,
    Count
};

struct MemoryStats
{
    std::size_t currentBytes = 0;
    std::size_t peakBytes = 0;
    std::size_t totalAllocatedBytes = 0;
    std::size_t allocationCount = 0;
    std::size_t freeCount = 0;
    std::array<std::size_t, static_cast<std::size_t>(MemoryClass::Count)> bytesByClass{};
    std::array<std::size_t, static_cast<std::size_t>(MemoryClass::Count)> peakBytesByClass{};
};
