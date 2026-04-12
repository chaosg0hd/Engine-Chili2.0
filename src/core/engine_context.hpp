#pragma once

class LoggerModule;
class IPlatformService;
class IGpuService;
class IRenderService;
class IResourceService;
class IJobService;
class IFileService;
class IMemoryService;
class IPrototypeService;

struct EngineContext
{
    bool IsRunning = true;
    float DeltaTime = 0.0f;
    double TotalTime = 0.0;
    LoggerModule* Logger = nullptr;
    IPlatformService* Platform = nullptr;
    IGpuService* Gpu = nullptr;
    IRenderService* Render = nullptr;
    IResourceService* Resources = nullptr;
    IJobService* Jobs = nullptr;
    IFileService* Files = nullptr;
    IMemoryService* Memory = nullptr;
    IPrototypeService* Prototypes = nullptr;
};
