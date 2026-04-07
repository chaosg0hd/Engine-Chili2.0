#include "studio_app.hpp"

bool StudioApp::Initialize()
{
    return m_host.Initialize();
}

void StudioApp::Run()
{
    m_host.Run();
}

void StudioApp::Shutdown()
{
    m_host.Shutdown();
}
