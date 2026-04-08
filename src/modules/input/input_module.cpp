#include "input_module.hpp"

#include "../../core/engine_context.hpp"
#include "../platform/iplatform_service.hpp"
#include "../platform/platform_window.hpp"

InputModule::InputModule() = default;

const char* InputModule::GetName() const
{
    return "Input";
}

bool InputModule::Initialize(EngineContext& context)
{
    if (m_initialized)
    {
        return true;
    }

    if (!m_platform)
    {
        m_platform = context.Platform;
    }

    m_currentKeys.fill(false);
    m_previousKeys.fill(false);
    m_pressedKeys.fill(false);
    m_releasedKeys.fill(false);
    m_currentMouseButtons.fill(false);
    m_previousMouseButtons.fill(false);
    m_pressedMouseButtons.fill(false);
    m_releasedMouseButtons.fill(false);
    m_mouseX = 0;
    m_mouseY = 0;
    m_previousMouseX = 0;
    m_previousMouseY = 0;
    m_mouseDeltaX = 0;
    m_mouseDeltaY = 0;
    m_mouseWheelDelta = 0;

    m_initialized = true;
    return true;
}

void InputModule::Startup(EngineContext& context)
{
    if (!m_initialized || m_started)
    {
        return;
    }

    if (!m_platform)
    {
        m_platform = context.Platform;
    }

    m_started = true;
}

void InputModule::Update(EngineContext& context, float deltaTime)
{
    (void)deltaTime;

    if (!m_platform)
    {
        m_platform = context.Platform;
    }

    if (!m_started || !m_platform)
    {
        return;
    }

    BeginFrameState();
    ProcessPlatformEvents();
}

void InputModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    m_platform = nullptr;
    m_currentKeys.fill(false);
    m_previousKeys.fill(false);
    m_pressedKeys.fill(false);
    m_releasedKeys.fill(false);
    m_currentMouseButtons.fill(false);
    m_previousMouseButtons.fill(false);
    m_pressedMouseButtons.fill(false);
    m_releasedMouseButtons.fill(false);
    m_mouseX = 0;
    m_mouseY = 0;
    m_previousMouseX = 0;
    m_previousMouseY = 0;
    m_mouseDeltaX = 0;
    m_mouseDeltaY = 0;
    m_mouseWheelDelta = 0;

    m_started = false;
    m_initialized = false;
}

bool InputModule::IsKeyDown(std::uint8_t key) const
{
    return m_currentKeys[key];
}

bool InputModule::WasKeyPressed(std::uint8_t key) const
{
    return m_pressedKeys[key];
}

bool InputModule::WasKeyReleased(std::uint8_t key) const
{
    return m_releasedKeys[key];
}

bool InputModule::IsMouseButtonDown(MouseButton button) const
{
    return m_currentMouseButtons[static_cast<std::size_t>(button)];
}

bool InputModule::WasMouseButtonPressed(MouseButton button) const
{
    return m_pressedMouseButtons[static_cast<std::size_t>(button)];
}

bool InputModule::WasMouseButtonReleased(MouseButton button) const
{
    return m_releasedMouseButtons[static_cast<std::size_t>(button)];
}

int InputModule::GetMouseX() const
{
    return m_mouseX;
}

int InputModule::GetMouseY() const
{
    return m_mouseY;
}

int InputModule::GetMouseDeltaX() const
{
    return m_mouseDeltaX;
}

int InputModule::GetMouseDeltaY() const
{
    return m_mouseDeltaY;
}

int InputModule::GetMouseWheelDelta() const
{
    return m_mouseWheelDelta;
}

bool InputModule::IsInitialized() const
{
    return m_initialized;
}

bool InputModule::IsStarted() const
{
    return m_started;
}

void InputModule::BeginFrameState()
{
    m_previousKeys = m_currentKeys;
    m_pressedKeys.fill(false);
    m_releasedKeys.fill(false);
    m_previousMouseButtons = m_currentMouseButtons;
    m_pressedMouseButtons.fill(false);
    m_releasedMouseButtons.fill(false);
    m_previousMouseX = m_mouseX;
    m_previousMouseY = m_mouseY;
    m_mouseDeltaX = 0;
    m_mouseDeltaY = 0;
    m_mouseWheelDelta = 0;
}

void InputModule::ProcessPlatformEvents()
{
    const auto& events = m_platform->GetEvents();

    for (const PlatformWindow::Event& event : events)
    {
        switch (event.type)
        {
        case PlatformWindow::Event::KeyDown:
        {
            const std::uint8_t key = static_cast<std::uint8_t>(event.a);
            if (!m_currentKeys[key])
            {
                m_pressedKeys[key] = true;
            }
            m_currentKeys[key] = true;
            break;
        }

        case PlatformWindow::Event::KeyUp:
        {
            const std::uint8_t key = static_cast<std::uint8_t>(event.a);
            m_currentKeys[key] = false;
            if (m_previousKeys[key])
            {
                m_releasedKeys[key] = true;
            }
            break;
        }

        case PlatformWindow::Event::MouseMove:
            m_mouseX = event.a;
            m_mouseY = event.b;
            m_mouseDeltaX = m_mouseX - m_previousMouseX;
            m_mouseDeltaY = m_mouseY - m_previousMouseY;
            break;

        case PlatformWindow::Event::MouseButtonDown:
        {
            const std::size_t button = static_cast<std::size_t>(event.a);
            if (button < m_currentMouseButtons.size())
            {
                if (!m_currentMouseButtons[button])
                {
                    m_pressedMouseButtons[button] = true;
                }

                m_currentMouseButtons[button] = true;
            }
            break;
        }

        case PlatformWindow::Event::MouseButtonUp:
        {
            const std::size_t button = static_cast<std::size_t>(event.a);
            if (button < m_currentMouseButtons.size())
            {
                m_currentMouseButtons[button] = false;
                if (m_previousMouseButtons[button])
                {
                    m_releasedMouseButtons[button] = true;
                }
            }
            break;
        }

        case PlatformWindow::Event::MouseWheel:
            m_mouseWheelDelta += event.a;
            break;

        default:
            break;
        }
    }
}
