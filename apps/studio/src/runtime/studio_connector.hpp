#pragma once

#include "runtime/runtime_commands.hpp"
#include "runtime/runtime_queries.hpp"
#include "runtime/studio_camera.hpp"

namespace studio_runtime
{
    class StudioConnector
    {
    public:
        StudioConnector(RuntimeWorld& world, StudioCamera& camera);

        RuntimeQueries& Queries();
        const RuntimeQueries& Queries() const;
        RuntimeCommands& Commands();
        StudioCamera& Camera();

    private:
        RuntimeWorld& m_world;
        RuntimeQueries m_queries;
        RuntimeCommands m_commands;
        StudioCamera& m_camera;
    };
}
