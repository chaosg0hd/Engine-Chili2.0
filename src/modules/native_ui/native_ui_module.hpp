#pragma once

#include "../../core/module.hpp"
#include "../../core/engine_core.hpp"
#include "native_button_host.hpp"

#include <cstdint>
#include <unordered_map>

class EngineContext;
class IPlatformService;

class NativeUiModule : public IModule
{
public:
    using ButtonHandle = std::uint32_t;

public:
    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    ButtonHandle CreateButton(const NativeButtonDesc& desc);
    bool DestroyButton(ButtonHandle handle);
    void DestroyAllButtons();
    bool SetButtonBounds(ButtonHandle handle, const NativeControlRect& rect);
    bool SetButtonText(ButtonHandle handle, const std::wstring& text);
    bool SetButtonVisible(ButtonHandle handle, bool visible);
    bool ConsumeButtonPressed(ButtonHandle handle);
    bool IsButtonOpen(ButtonHandle handle) const;

private:
    struct ButtonEntry
    {
        NativeButtonDesc desc;
        NativeButtonHost host;
    };

private:
    HWND GetEngineWindowHandle() const;
    ButtonEntry* FindButton(ButtonHandle handle);
    const ButtonEntry* FindButton(ButtonHandle handle) const;

private:
    IPlatformService* m_platform = nullptr;
    std::unordered_map<ButtonHandle, ButtonEntry> m_buttons;
    ButtonHandle m_nextHandle = 1U;
    bool m_initialized = false;
};
