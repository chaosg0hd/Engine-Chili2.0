#include "native_ui_module.hpp"

#include "../platform/platform_module.hpp"

const char* NativeUiModule::GetName() const
{
    return "NativeUiModule";
}

bool NativeUiModule::Initialize(EngineContext& context)
{
    (void)context;
    m_initialized = true;
    return true;
}

void NativeUiModule::Startup(EngineContext& context)
{
    (void)context;
}

void NativeUiModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;
}

void NativeUiModule::Shutdown(EngineContext& context)
{
    (void)context;
    DestroyAllButtons();
    m_initialized = false;
}

void NativeUiModule::SetPlatformModule(PlatformModule* platform)
{
    m_platform = platform;
}

NativeUiModule::ButtonHandle NativeUiModule::CreateButton(const NativeButtonDesc& desc)
{
    if (!m_initialized || !m_platform)
    {
        return 0U;
    }

    const HWND engineWindowHandle = GetEngineWindowHandle();
    if (!engineWindowHandle)
    {
        return 0U;
    }

    const ButtonHandle handle = m_nextHandle++;
    auto [iterator, inserted] = m_buttons.try_emplace(handle);
    if (!inserted)
    {
        return 0U;
    }

    iterator->second.desc = desc;
    if (!iterator->second.host.Initialize(engineWindowHandle, desc))
    {
        m_buttons.erase(iterator);
        return 0U;
    }

    return handle;
}

bool NativeUiModule::DestroyButton(ButtonHandle handle)
{
    ButtonEntry* button = FindButton(handle);
    if (!button)
    {
        return false;
    }

    button->host.Shutdown();
    m_buttons.erase(handle);
    return true;
}

void NativeUiModule::DestroyAllButtons()
{
    for (auto& [handle, button] : m_buttons)
    {
        (void)handle;
        button.host.Shutdown();
    }

    m_buttons.clear();
}

bool NativeUiModule::SetButtonBounds(ButtonHandle handle, const NativeControlRect& rect)
{
    ButtonEntry* button = FindButton(handle);
    if (!button)
    {
        return false;
    }

    button->desc.rect = rect;
    return button->host.SetBounds(rect);
}

bool NativeUiModule::SetButtonText(ButtonHandle handle, const std::wstring& text)
{
    ButtonEntry* button = FindButton(handle);
    if (!button)
    {
        return false;
    }

    button->desc.text = text;
    return button->host.SetText(text);
}

bool NativeUiModule::SetButtonVisible(ButtonHandle handle, bool visible)
{
    ButtonEntry* button = FindButton(handle);
    if (!button)
    {
        return false;
    }

    button->desc.visible = visible;
    return button->host.SetVisible(visible);
}

bool NativeUiModule::ConsumeButtonPressed(ButtonHandle handle)
{
    ButtonEntry* button = FindButton(handle);
    return button ? button->host.ConsumePressed() : false;
}

bool NativeUiModule::IsButtonOpen(ButtonHandle handle) const
{
    const ButtonEntry* button = FindButton(handle);
    return button ? button->host.IsOpen() : false;
}

HWND NativeUiModule::GetEngineWindowHandle() const
{
    return m_platform ? m_platform->GetWindowHandle() : nullptr;
}

NativeUiModule::ButtonEntry* NativeUiModule::FindButton(ButtonHandle handle)
{
    const auto iterator = m_buttons.find(handle);
    return iterator != m_buttons.end() ? &iterator->second : nullptr;
}

const NativeUiModule::ButtonEntry* NativeUiModule::FindButton(ButtonHandle handle) const
{
    const auto iterator = m_buttons.find(handle);
    return iterator != m_buttons.end() ? &iterator->second : nullptr;
}
