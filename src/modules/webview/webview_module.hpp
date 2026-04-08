#pragma once

#include "../../core/module.hpp"
#include "../../core/engine_core.hpp"
#include "web_dialog_host.hpp"

#include <cstdint>
#include <unordered_map>

class EngineContext;

class PlatformModule;

class WebViewModule : public IModule
{
public:
    using DialogHandle = std::uint32_t;

public:
    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    DialogHandle CreateWebDialogInstance(const WebDialogDesc& desc);
    bool DestroyWebDialogInstance(DialogHandle handle);
    void DestroyAllDialogs();
    bool SetDialogContentPath(DialogHandle handle, const std::string& contentPath);
    bool SetDialogDockMode(DialogHandle handle, WebDialogDockMode dockMode, int dockSize);
    bool SetDialogBounds(DialogHandle handle, const WebDialogRect& rect);
    bool SetDialogVisible(DialogHandle handle, bool visible);
    bool IsDialogReady(DialogHandle handle) const;
    bool IsDialogOpen(DialogHandle handle) const;
    WebDialogRect GetDialogBounds(DialogHandle handle) const;

private:
    struct DialogEntry
    {
        WebDialogDesc desc;
        WebDialogHost host;
    };

private:
    HWND GetEngineWindowHandle() const;
    RECT GetEngineClientRect() const;
    DialogEntry* FindDialog(DialogHandle handle);
    const DialogEntry* FindDialog(DialogHandle handle) const;

private:
    PlatformModule* m_platform = nullptr;
    std::unordered_map<DialogHandle, DialogEntry> m_dialogs;
    DialogHandle m_nextHandle = 1U;
    bool m_initialized = false;
};
