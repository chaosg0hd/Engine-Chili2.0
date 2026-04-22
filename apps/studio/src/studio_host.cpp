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

namespace
{
    constexpr int kStudioTopBarHeight = 76;
    constexpr int kCoreToolsDockWidth = 208;
    constexpr int kStudioContentMargin = 12;
}

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

    if (!InitializeTopBarDialog())
    {
        m_bridge.LogError("Studio: failed to initialize the engine-owned Studio top bar.");
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
            m_bridge.GetCapabilities().ui->ConsumeNativeButtonPressed(m_nativeButtonHandle))
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

    if (m_topBarDialogHandle != 0U)
    {
        m_bridge.GetCapabilities().ui->DestroyWebDialog(m_topBarDialogHandle);
        m_topBarDialogHandle = 0U;
    }

    if (m_coreToolsDialogHandle != 0U)
    {
        m_bridge.GetCapabilities().ui->DestroyWebDialog(m_coreToolsDialogHandle);
        m_coreToolsDialogHandle = 0U;
    }

    if (m_nativeButtonHandle != 0U)
    {
        m_bridge.GetCapabilities().ui->DestroyNativeButton(m_nativeButtonHandle);
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
    m_bridge.LogInfo(
        std::string("Studio: top bar entry = ") +
        m_bridge.GetStudioTopBarContentPath());
    m_bridge.LogInfo("Studio: shell chrome = docked-top and docked-left via engine web dialog API.");
    m_bridge.LogInfo("Studio: native UI sample button is active via engine native UI API.");
    m_bridge.LogInfo("Studio: transport scaffolding is present under src/transport and currently inactive.");
}

bool StudioHost::InitializeTopBarDialog()
{
    WebDialogDesc dialogDesc;
    dialogDesc.name = "StudioTopBar";
    dialogDesc.title = L"Studio Top Bar";
    dialogDesc.contentPath = m_bridge.GetStudioTopBarContentPath();
    dialogDesc.dockMode = WebDialogDockMode::Top;
    dialogDesc.dockSize = kStudioTopBarHeight;
    dialogDesc.visible = true;
    dialogDesc.resizable = false;

    m_topBarDialogHandle = m_bridge.GetCapabilities().ui->CreateWebDialog(dialogDesc);
    return m_topBarDialogHandle != 0U;
}

bool StudioHost::InitializeCoreToolsDialog()
{
    WebDialogDesc dialogDesc;
    dialogDesc.name = "CoreTools";
    dialogDesc.title = L"CoreTools";
    dialogDesc.contentPath = m_bridge.GetCoreToolsContentPath();
    dialogDesc.dockMode = WebDialogDockMode::Left;
    dialogDesc.dockSize = kCoreToolsDockWidth;
    dialogDesc.visible = true;
    dialogDesc.resizable = false;

    m_coreToolsDialogHandle = m_bridge.GetCapabilities().ui->CreateWebDialog(dialogDesc);
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

    m_nativeButtonHandle = m_bridge.GetCapabilities().ui->CreateNativeButton(buttonDesc);
    return m_nativeButtonHandle != 0U;
}

void StudioHost::UpdateNativeButtonLayout()
{
    if (m_nativeButtonHandle == 0U)
    {
        return;
    }

    m_bridge.GetCapabilities().ui->SetNativeButtonBounds(m_nativeButtonHandle, ComputeNativeButtonRect());
}

NativeControlRect StudioHost::ComputeNativeButtonRect() const
{
    const int clientWidth = m_bridge.GetCapabilities().window->GetWindowWidth();
    const int clientHeight = m_bridge.GetCapabilities().window->GetWindowHeight();
    const int buttonWidth = 112;
    const int buttonHeight = 36;

    NativeControlRect rect;
    rect.x = kCoreToolsDockWidth + kStudioContentMargin;
    rect.y = kStudioTopBarHeight + kStudioContentMargin;
    rect.width = buttonWidth;
    rect.height = buttonHeight;

    if (clientWidth > 0)
    {
        const int maxX = std::max(kStudioContentMargin, clientWidth - buttonWidth - kStudioContentMargin);
        if (rect.x > maxX)
        {
            rect.x = maxX;
        }
    }

    if (clientHeight > 0)
    {
        const int maxY = std::max(kStudioContentMargin, clientHeight - buttonHeight - kStudioContentMargin);
        if (rect.y > maxY)
        {
            rect.y = maxY;
        }
    }

    return rect;
}
