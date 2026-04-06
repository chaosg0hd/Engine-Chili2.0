#include "diagnostics_module.hpp"

#include "../../core/engine_context.hpp"

#include <windows.h>
#include <psapi.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string FormatBytes(unsigned long long bytes)
    {
        const char* units[] = { "B", "KB", "MB", "GB", "TB", "PB" };

        double value = static_cast<double>(bytes);
        int unitIndex = 0;

        while (value >= 1024.0 && unitIndex < 5)
        {
            value /= 1024.0;
            ++unitIndex;
        }

        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << value << ' ' << units[unitIndex];
        return stream.str();
    }

    std::string GetArchitectureName(WORD architecture)
    {
        switch (architecture)
        {
            case PROCESSOR_ARCHITECTURE_AMD64: return "x64";
            case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
            case PROCESSOR_ARCHITECTURE_ARM:   return "ARM";
            case PROCESSOR_ARCHITECTURE_ARM64: return "ARM64";
            default:                           return "Unknown";
        }
    }

    unsigned int CountBits(ULONG_PTR mask)
    {
        unsigned int count = 0;

        while (mask != 0)
        {
            count += static_cast<unsigned int>(mask & 1);
            mask >>= 1;
        }

        return count;
    }

    unsigned int GetPhysicalCoreCount()
    {
        DWORD requiredLength = 0;
        GetLogicalProcessorInformation(nullptr, &requiredLength);

        if (requiredLength == 0)
        {
            return 0;
        }

        std::vector<unsigned char> buffer(requiredLength);
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION* info =
            reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(buffer.data());

        if (!GetLogicalProcessorInformation(info, &requiredLength))
        {
            return 0;
        }

        const DWORD count = requiredLength / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        unsigned int physicalCores = 0;

        for (DWORD i = 0; i < count; ++i)
        {
            if (info[i].Relationship == RelationProcessorCore)
            {
                ++physicalCores;
            }
        }

        return physicalCores;
    }

    void PrintResourceDiagnostics(double uptimeSeconds, unsigned long long loopCount)
    {
        SYSTEM_INFO systemInfo{};
        GetNativeSystemInfo(&systemInfo);

        const unsigned int logicalProcessors =
            static_cast<unsigned int>(systemInfo.dwNumberOfProcessors);
        const unsigned int physicalCores = GetPhysicalCoreCount();
        const std::string architecture = GetArchitectureName(systemInfo.wProcessorArchitecture);
        const unsigned int pageSize = static_cast<unsigned int>(systemInfo.dwPageSize);
        const unsigned int allocationGranularity =
            static_cast<unsigned int>(systemInfo.dwAllocationGranularity);

        DWORD_PTR processAffinityMask = 0;
        DWORD_PTR systemAffinityMask = 0;
        unsigned int usableProcessors = logicalProcessors;

        if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask))
        {
            usableProcessors = CountBits(processAffinityMask);
        }

        MEMORYSTATUSEX memoryStatus{};
        memoryStatus.dwLength = sizeof(memoryStatus);

        unsigned long long totalPhysical = 0;
        unsigned long long availablePhysical = 0;
        unsigned long long totalVirtual = 0;
        unsigned long long availableVirtual = 0;

        if (GlobalMemoryStatusEx(&memoryStatus))
        {
            totalPhysical = static_cast<unsigned long long>(memoryStatus.ullTotalPhys);
            availablePhysical = static_cast<unsigned long long>(memoryStatus.ullAvailPhys);
            totalVirtual = static_cast<unsigned long long>(memoryStatus.ullTotalVirtual);
            availableVirtual = static_cast<unsigned long long>(memoryStatus.ullAvailVirtual);
        }

        PROCESS_MEMORY_COUNTERS_EX processMemory{};
        unsigned long long workingSet = 0;
        unsigned long long privateUsage = 0;

        if (GetProcessMemoryInfo(
                GetCurrentProcess(),
                reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&processMemory),
                sizeof(processMemory)))
        {
            workingSet = static_cast<unsigned long long>(processMemory.WorkingSetSize);
            privateUsage = static_cast<unsigned long long>(processMemory.PrivateUsage);
        }

        const char* rootPath = "C:\\";
        ULARGE_INTEGER freeBytesAvailable{};
        ULARGE_INTEGER totalBytes{};
        ULARGE_INTEGER totalFreeBytes{};

        unsigned long long diskFreeAvailable = 0;
        unsigned long long diskTotal = 0;
        unsigned long long diskTotalFree = 0;

        if (GetDiskFreeSpaceExA(rootPath, &freeBytesAvailable, &totalBytes, &totalFreeBytes))
        {
            diskFreeAvailable = freeBytesAvailable.QuadPart;
            diskTotal = totalBytes.QuadPart;
            diskTotalFree = totalFreeBytes.QuadPart;
        }

        std::cout << "\n==== Engine Diagnostics ====\n";

        std::cout << "Runtime:\n";
        std::cout << "  Uptime                  : "
                  << std::fixed << std::setprecision(2)
                  << uptimeSeconds << " s\n";
        std::cout << "  Loop Count              : " << loopCount << "\n";

        std::cout << "CPU / Hardware:\n";
        std::cout << "  Architecture            : " << architecture << "\n";
        std::cout << "  Physical Cores          : " << physicalCores << "\n";
        std::cout << "  Logical Processors      : " << logicalProcessors << "\n";
        std::cout << "  Usable Processors       : " << usableProcessors << "\n";

        std::cout << "Memory:\n";
        std::cout << "  Total Physical RAM      : " << FormatBytes(totalPhysical) << "\n";
        std::cout << "  Available Physical RAM  : " << FormatBytes(availablePhysical) << "\n";
        std::cout << "  Total Virtual Memory    : " << FormatBytes(totalVirtual) << "\n";
        std::cout << "  Available Virtual Memory: " << FormatBytes(availableVirtual) << "\n";
        std::cout << "  Process Working Set     : " << FormatBytes(workingSet) << "\n";
        std::cout << "  Process Private Usage   : " << FormatBytes(privateUsage) << "\n";

        std::cout << "Storage:\n";
        std::cout << "  Root Path               : " << rootPath << "\n";
        std::cout << "  Free Available Space    : " << FormatBytes(diskFreeAvailable) << "\n";
        std::cout << "  Total Capacity          : " << FormatBytes(diskTotal) << "\n";
        std::cout << "  Total Free Space        : " << FormatBytes(diskTotalFree) << "\n";

        std::cout << "System:\n";
        std::cout << "  Page Size               : " << pageSize << " bytes\n";
        std::cout << "  Allocation Granularity  : " << allocationGranularity << " bytes\n";

        std::cout << "============================\n";
    }
}

DiagnosticsModule::DiagnosticsModule()
    : m_initialized(false),
      m_started(false),
      m_uptimeSeconds(0.0),
      m_loopCount(0)
{
}

const char* DiagnosticsModule::GetName() const
{
    return "Diagnostics";
}

bool DiagnosticsModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    m_initialized = true;
    return true;
}

void DiagnosticsModule::Startup(EngineContext& context)
{
    if (!m_initialized || m_started)
    {
        return;
    }

    m_uptimeSeconds = context.TotalTime;
    m_loopCount = 0;
    m_started = true;
}

void DiagnosticsModule::Update(EngineContext& context, float deltaTime)
{
    if (!m_started)
    {
        return;
    }

    (void)deltaTime;

    m_uptimeSeconds = context.TotalTime;
    ++m_loopCount;
}

void DiagnosticsModule::EmitReport() const
{
    if (!m_started)
    {
        return;
    }

    PrintResourceDiagnostics(m_uptimeSeconds, m_loopCount);
}

void DiagnosticsModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    m_started = false;
    m_initialized = false;
}

double DiagnosticsModule::GetUptimeSeconds() const
{
    return m_uptimeSeconds;
}

unsigned long long DiagnosticsModule::GetLoopCount() const
{
    return m_loopCount;
}

bool DiagnosticsModule::IsInitialized() const
{
    return m_initialized;
}

bool DiagnosticsModule::IsStarted() const
{
    return m_started;
}
