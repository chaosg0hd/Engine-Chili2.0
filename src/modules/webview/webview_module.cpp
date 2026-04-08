#include "webview_module.hpp"

#include "../../core/engine_context.hpp"
#include "../platform/iplatform_service.hpp"

const char* WebViewModule::GetName() const
{
    return "WebView";
}

bool WebViewModule::Initialize(EngineContext& context)
{
    if (!m_platform)
    {
        m_platform = context.Platform;
    }

    m_initialized = true;
    return true;
}

void WebViewModule::Startup(EngineContext& context)
{
    if (!m_platform)
    {
        m_platform = context.Platform;
    }
}

void WebViewModule::Update(EngineContext& context, float deltaTime)
{
    (void)deltaTime;

    if (!m_platform)
    {
        m_platform = context.Platform;
    }

    if (!m_platform)
    {
        return;
    }

    const RECT clientRect = GetEngineClientRect();
    for (auto& dialogPair : m_dialogs)
    {
        dialogPair.second.host.TickLayout(clientRect);
    }
}

void WebViewModule::Shutdown(EngineContext& context)
{
    (void)context;
    DestroyAllDialogs();
    m_initialized = false;
}

WebViewModule::DialogHandle WebViewModule::CreateWebDialogInstance(const WebDialogDesc& desc)
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

    const DialogHandle handle = m_nextHandle++;
    auto [it, inserted] = m_dialogs.try_emplace(handle);
    if (!inserted)
    {
        return 0U;
    }

    DialogEntry& entry = it->second;
    entry.desc = desc;
    if (!entry.host.Initialize(engineWindowHandle, desc))
    {
        m_dialogs.erase(it);
        return 0U;
    }

    return handle;
}

bool WebViewModule::DestroyWebDialogInstance(DialogHandle handle)
{
    auto it = m_dialogs.find(handle);
    if (it == m_dialogs.end())
    {
        return false;
    }

    it->second.host.Shutdown();
    m_dialogs.erase(it);
    return true;
}

void WebViewModule::DestroyAllDialogs()
{
    for (auto& dialogPair : m_dialogs)
    {
        dialogPair.second.host.Shutdown();
    }

    m_dialogs.clear();
}

bool WebViewModule::SetDialogContentPath(DialogHandle handle, const std::string& contentPath)
{
    DialogEntry* dialog = FindDialog(handle);
    if (!dialog)
    {
        return false;
    }

    dialog->desc.contentPath = contentPath;
    return dialog->host.SetContentPath(contentPath);
}

bool WebViewModule::SetDialogDockMode(DialogHandle handle, WebDialogDockMode dockMode, int dockSize)
{
    DialogEntry* dialog = FindDialog(handle);
    if (!dialog)
    {
        return false;
    }

    dialog->desc.dockMode = dockMode;
    dialog->desc.dockSize = dockSize;
    return dialog->host.SetDockMode(dockMode, dockSize);
}

bool WebViewModule::SetDialogBounds(DialogHandle handle, const WebDialogRect& rect)
{
    DialogEntry* dialog = FindDialog(handle);
    if (!dialog)
    {
        return false;
    }

    dialog->desc.rect = rect;
    return dialog->host.SetBounds(rect);
}

bool WebViewModule::SetDialogVisible(DialogHandle handle, bool visible)
{
    DialogEntry* dialog = FindDialog(handle);
    if (!dialog)
    {
        return false;
    }

    dialog->desc.visible = visible;
    return dialog->host.SetVisible(visible);
}

bool WebViewModule::IsDialogReady(DialogHandle handle) const
{
    const DialogEntry* dialog = FindDialog(handle);
    return dialog ? dialog->host.IsReady() : false;
}

bool WebViewModule::IsDialogOpen(DialogHandle handle) const
{
    const DialogEntry* dialog = FindDialog(handle);
    return dialog ? dialog->host.IsOpen() : false;
}

WebDialogRect WebViewModule::GetDialogBounds(DialogHandle handle) const
{
    const DialogEntry* dialog = FindDialog(handle);
    return dialog ? dialog->host.GetBounds() : WebDialogRect{};
}

HWND WebViewModule::GetEngineWindowHandle() const
{
    return m_platform ? m_platform->GetWindowHandle() : nullptr;
}

RECT WebViewModule::GetEngineClientRect() const
{
    RECT clientRect{};
    const HWND hwnd = GetEngineWindowHandle();
    if (hwnd)
    {
        GetClientRect(hwnd, &clientRect);
    }

    return clientRect;
}

WebViewModule::DialogEntry* WebViewModule::FindDialog(DialogHandle handle)
{
    auto it = m_dialogs.find(handle);
    return (it != m_dialogs.end()) ? &it->second : nullptr;
}

const WebViewModule::DialogEntry* WebViewModule::FindDialog(DialogHandle handle) const
{
    auto it = m_dialogs.find(handle);
    return (it != m_dialogs.end()) ? &it->second : nullptr;
}
