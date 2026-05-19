#pragma once

#include "runtime/runtime_world.hpp"
#include "runtime/studio_camera.hpp"

#include <string>

namespace studio_runtime
{
    struct DefaultSceneTemplatePresence
    {
        bool hasOrigin = false;
        bool hasAxes = false;
        bool hasGrid = true;
        bool hasCamera = false;
    };

    void ApplyDefaultSceneTemplate(RuntimeWorld& world, StudioCamera& camera);
    DefaultSceneTemplatePresence ValidateDefaultSceneTemplate(const RuntimeWorld& world, const StudioCamera& camera);
    std::string BuildDefaultSceneTemplateReport(const DefaultSceneTemplatePresence& presence);
}

