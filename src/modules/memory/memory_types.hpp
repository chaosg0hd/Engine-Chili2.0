#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

enum class MemoryClass : std::size_t
{
    Unknown = 0,
    Core,
    Module,
    Frame,
    Upload,
    Resource,
    Persistent,
    Temporary,
    Job,
    Debug,
    Count
};

enum class MemoryLifetime : std::uint8_t
{
    Unknown = 0,
    Persistent,
    Transient,
    Diagnostic
};

inline constexpr const char* GetMemoryClassName(MemoryClass memoryClass)
{
    switch (memoryClass)
    {
    case MemoryClass::Unknown:    return "Unknown";
    case MemoryClass::Core:       return "Core";
    case MemoryClass::Module:     return "Module";
    case MemoryClass::Frame:      return "Frame";
    case MemoryClass::Upload:     return "Upload";
    case MemoryClass::Resource:   return "Resource";
    case MemoryClass::Persistent: return "Persistent";
    case MemoryClass::Temporary:  return "Temporary";
    case MemoryClass::Job:        return "Job";
    case MemoryClass::Debug:      return "Debug";
    case MemoryClass::Count:      return "Count";
    default:                      return "Invalid";
    }
}

inline constexpr MemoryLifetime GetMemoryLifetime(MemoryClass memoryClass)
{
    switch (memoryClass)
    {
    case MemoryClass::Core:
    case MemoryClass::Module:
    case MemoryClass::Resource:
    case MemoryClass::Persistent:
        return MemoryLifetime::Persistent;

    case MemoryClass::Frame:
    case MemoryClass::Upload:
    case MemoryClass::Temporary:
    case MemoryClass::Job:
        return MemoryLifetime::Transient;

    case MemoryClass::Debug:
        return MemoryLifetime::Diagnostic;

    case MemoryClass::Unknown:
    case MemoryClass::Count:
    default:
        return MemoryLifetime::Unknown;
    }
}

inline constexpr bool IsTransientMemoryClass(MemoryClass memoryClass)
{
    return GetMemoryLifetime(memoryClass) == MemoryLifetime::Transient;
}

inline constexpr bool IsPersistentMemoryClass(MemoryClass memoryClass)
{
    return GetMemoryLifetime(memoryClass) == MemoryLifetime::Persistent;
}

inline constexpr bool IsDiagnosticMemoryClass(MemoryClass memoryClass)
{
    return GetMemoryLifetime(memoryClass) == MemoryLifetime::Diagnostic;
}

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
