#pragma once

#include "prototypes/compiler/progressive_hex_render_compiler.hpp"
#include "prototypes/compiler/progressive_hex_render_strategy.hpp"
#include "prototypes/presentation/frame.hpp"
#include "prototypes/systems/moving_cube_sample_scene.hpp"
#include "modules/render/progressive_hex_render_controller.hpp"

#include <string>

class EngineCore;

class SandboxApp
{
public:
    bool Run();

private:
    struct SandboxState
    {
        unsigned int theoreticalWorkIterations = 0U;
        bool maxDepthSignalLogged = false;
        bool passLogWritten = false;
        bool traversalLogWritten = false;
    };

private:
    void UpdateFrame(EngineCore& core);
    void UpdateLogic(EngineCore& core);
    void ConfigureStrategy();
    void UpdateCenterPassLog(EngineCore& core);
    void WriteTraversalLog(EngineCore& core);

private:
    SandboxState m_state;
    MovingCubeSampleScenePrototype m_sampleScene;
    render::ProgressiveHexRenderController m_renderController;
};
