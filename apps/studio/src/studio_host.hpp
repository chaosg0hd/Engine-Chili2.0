#pragma once

#include "bridge/engine_bridge.hpp"
#include "commands/command_router.hpp"

class StudioHost
{
public:
    bool Initialize();
    void Run();
    void Shutdown();

private:
    void LogStudioShellStatus();
    bool InitializeCoreToolsDialog();
    bool InitializeNativeButton();
    void UpdateNativeButtonLayout();
    NativeControlRect ComputeNativeButtonRect() const;

private:
    EngineBridge m_bridge;
    CommandRouter m_commandRouter;
    EngineCore::WebDialogHandle m_coreToolsDialogHandle = 0U;
    EngineCore::NativeButtonHandle m_nativeButtonHandle = 0U;
    bool m_initialized = false;
};
