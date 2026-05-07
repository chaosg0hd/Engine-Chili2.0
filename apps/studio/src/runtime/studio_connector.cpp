#include "runtime/studio_connector.hpp"

namespace studio_runtime
{

    StudioConnector::StudioConnector(RuntimeWorld& world, StudioCamera& camera)
        : m_world(world),
          m_queries(world),
          m_commands(world),
          m_camera(camera)
    {
    }

    RuntimeQueries& StudioConnector::Queries()
    {
        return m_queries;
    }

    const RuntimeQueries& StudioConnector::Queries() const
    {
        return m_queries;
    }

    RuntimeCommands& StudioConnector::Commands()
    {
        return m_commands;
    }

    StudioCamera& StudioConnector::Camera()
    {
        return m_camera;
    }

}
