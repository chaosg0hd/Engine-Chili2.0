#include "studio_host.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <algorithm>
#include <string>

bool StudioHost::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    if (!m_bridge.Initialize())
    {
        return false;
    }

    const HWND windowHandle = m_bridge.GetNativeWindowHandle();
    if (!windowHandle)
    {
        m_bridge.LogError("Studio: native host window is not available.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeCoreToolsDialog())
    {
        m_bridge.LogError("Studio: failed to initialize the engine-owned CoreTools dialog.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeNativeButton())
    {
        m_bridge.LogError("Studio: failed to initialize the engine-owned native button.");
        m_bridge.Shutdown();
        return false;
    }

    m_commandRouter.Bind(&m_bridge);
    LogStudioShellStatus();
    m_initialized = true;
    return true;
}

void StudioHost::Run()
{
    if (!m_initialized)
    {
        return;
    }

    m_bridge.LogInfo("Studio: native host entering main loop.");
    m_bridge.LogInfo("Studio: close the native window or press Escape to stop the studio host.");

    while (!m_bridge.ShouldExit())
    {
        if (!m_bridge.Tick())
        {
            break;
        }

        UpdateNativeButtonLayout();

        if (m_nativeButtonHandle != 0U &&
            m_bridge.GetCore().ConsumeNativeButtonPressed(m_nativeButtonHandle))
        {
            m_bridge.LogInfo("Studio: native sample button pressed.");
        }
    }
}

void StudioHost::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    if (m_coreToolsDialogHandle != 0U)
    {
        m_bridge.GetCore().DestroyWebDialog(m_coreToolsDialogHandle);
        m_coreToolsDialogHandle = 0U;
    }

    if (m_nativeButtonHandle != 0U)
    {
        m_bridge.GetCore().DestroyNativeButton(m_nativeButtonHandle);
        m_nativeButtonHandle = 0U;
    }

    m_bridge.Shutdown();
    m_initialized = false;
}

void StudioHost::LogStudioShellStatus()
{
    m_bridge.LogInfo("Studio: shell boot complete.");
    m_bridge.LogInfo("Studio: native outer window is active.");
    m_bridge.LogInfo(
        std::string("Studio: CoreTools entry = ") +
        m_bridge.GetCoreToolsContentPath());
    m_bridge.LogInfo("Studio: CoreTools dialog mode = docked-left via engine web dialog API.");
    m_bridge.LogInfo("Studio: native UI sample button is active via engine native UI API.");
    m_bridge.LogInfo("Studio: transport scaffolding is present under src/transport and currently inactive.");
}

bool StudioHost::InitializeCoreToolsDialog()
{
    WebDialogDesc dialogDesc;
    dialogDesc.name = "CoreTools";
    dialogDesc.title = L"CoreTools";
    dialogDesc.contentPath = m_bridge.GetCoreToolsContentPath();
    dialogDesc.dockMode = WebDialogDockMode::Left;
    dialogDesc.dockSize = 200;
    dialogDesc.visible = true;
    dialogDesc.resizable = false;

    m_coreToolsDialogHandle = m_bridge.GetCore().CreateWebDialog(dialogDesc);
    return m_coreToolsDialogHandle != 0U;
}

bool StudioHost::InitializeNativeButton()
{
    NativeButtonDesc buttonDesc;
    buttonDesc.name = "StudioSampleButton";
    buttonDesc.text = L"Native Tool";
    buttonDesc.rect = ComputeNativeButtonRect();
    buttonDesc.visible = true;
    buttonDesc.enabled = true;

    m_nativeButtonHandle = m_bridge.GetCore().CreateNativeButton(buttonDesc);
    return m_nativeButtonHandle != 0U;
}

void StudioHost::UpdateNativeButtonLayout()
{
    if (m_nativeButtonHandle == 0U)
    {
        return;
    }

    m_bridge.GetCore().SetNativeButtonBounds(m_nativeButtonHandle, ComputeNativeButtonRect());
}

NativeControlRect StudioHost::ComputeNativeButtonRect() const
{
    const int dockWidth = 200;
    const int clientWidth = m_bridge.GetCore().GetWindowWidth();
    const int buttonWidth = 112;
    const int buttonHeight = 36;
    const int margin = 12;

    NativeControlRect rect;
    rect.x = dockWidth + margin;
    rect.y = margin;
    rect.width = buttonWidth;
    rect.height = buttonHeight;

    if (clientWidth > 0)
    {
        const int maxX = std::max(margin, clientWidth - buttonWidth - margin);
        if (rect.x > maxX)
        {
            rect.x = maxX;
        }
    }

    return rect;
}
