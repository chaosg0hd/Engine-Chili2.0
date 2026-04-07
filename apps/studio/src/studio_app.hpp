#pragma once

#include "studio_host.hpp"

class StudioApp
{
public:
    bool Initialize();
    void Run();
    void Shutdown();

private:
    StudioHost m_host;
};
