#pragma once

#include "bridge/engine_bridge.hpp"
#include "runtime/studio_runtime_host.hpp"
#include "scene_manager.hpp"

#include <string>

class GameHost
{
public:
    bool Initialize();
    void Run();
    void Shutdown();

private:
    bool LoadScene(int index);
    void TickGame();

    EngineBridge m_bridge;
    studio_runtime::StudioRuntimeHost m_runtimeHost;
    SceneManager m_sceneManager;
    bool m_initialized = false;
};
