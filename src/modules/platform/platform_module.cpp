#include "platform_module.hpp"

#include "../../core/engine_context.hpp"
#include "platform_window.hpp"

PlatformModule::PlatformModule()
    : m_initialized(false),
      m_started(false),
      m_platformName("Windows"),
      m_window(nullptr)
{
}

PlatformModule::~PlatformModule()
{
    delete m_window;
    m_window = nullptr;
}

const char* PlatformModule::GetName() const
{
    return "Platform";
}

bool PlatformModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    m_window = new PlatformWindow();

    if (!m_window->Create(L"Project Engine", 1280, 720))
    {
        delete m_window;
        m_window = nullptr;
        return false;
    }

    m_initialized = true;
    return true;
}

void PlatformModule::Startup(EngineContext& context)
{
    (void)context;

    if (!m_initialized || m_started)
    {
        return;
    }

    if (m_window)
    {
        m_window->Show(SW_MAXIMIZE);
    }

    m_started = true;
}

void PlatformModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;

    if (!m_started || !m_window)
    {
        return;
    }

    m_window->PollEvents();
}

void PlatformModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    delete m_window;
    m_window = nullptr;

    m_started = false;
    m_initialized = false;
}

bool PlatformModule::IsInitialized() const
{
    return m_initialized;
}

bool PlatformModule::IsStarted() const
{
    return m_started;
}

bool PlatformModule::IsWindowOpen() const
{
    return (m_window != nullptr) ? m_window->IsOpen() : false;
}

bool PlatformModule::IsWindowActive() const
{
    return (m_window != nullptr) ? m_window->IsActive() : false;
}

HWND PlatformModule::GetWindowHandle() const
{
    return m_window ? m_window->GetHandle() : nullptr;
}

int PlatformModule::GetWindowWidth() const
{
    return m_window ? m_window->GetClientWidth() : 0;
}

int PlatformModule::GetWindowHeight() const
{
    return m_window ? m_window->GetClientHeight() : 0;
}

void PlatformModule::PollEvents()
{
    if (m_window)
    {
        m_window->PollEvents();
    }
}

const std::vector<PlatformWindow::Event>& PlatformModule::GetEvents() const
{
    static const std::vector<PlatformWindow::Event> emptyEvents;

    if (!m_window)
    {
        return emptyEvents;
    }

    return m_window->GetEvents();
}

void PlatformModule::ClearEvents()
{
    if (m_window)
    {
        m_window->ClearEvents();
    }
}

const std::string& PlatformModule::GetPlatformName() const
{
    return m_platformName;
}

void PlatformModule::SetOverlayText(const std::wstring& text)
{
    if (m_window)
    {
        m_window->SetOverlayText(text);
    }
}

const std::wstring& PlatformModule::GetOverlayText() const
{
    static const std::wstring emptyText;
    return m_window ? m_window->GetOverlayText() : emptyText;
}

void PlatformModule::SetWindowTitle(const std::wstring& title)
{
    if (m_window)
    {
        m_window->SetTitle(title);
    }
}
