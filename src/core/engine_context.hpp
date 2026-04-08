#pragma once

class LoggerModule;
class PlatformModule;
class IGpuService;
class ResourceModule;

struct EngineContext
{
    bool IsRunning = true;
    float DeltaTime = 0.0f;
    double TotalTime = 0.0;
    LoggerModule* Logger = nullptr;
    PlatformModule* Platform = nullptr;
    IGpuService* Gpu = nullptr;
    ResourceModule* Resources = nullptr;
};
