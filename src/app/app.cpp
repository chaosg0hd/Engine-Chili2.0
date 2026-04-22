#include "app.hpp"

#include "app_capabilities.hpp"
#include "../core/engine_core.hpp"

#include <string>

bool App::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    AppCapabilities& capabilities = core.GetAppCapabilities();

    capabilities.logging->Info(
        std::string("App: Generic runner ready | file logging = ") +
        (capabilities.logging->IsFileLoggingAvailable() ? "true" : "false") +
        " | log path = " +
        capabilities.logging->GetLogFilePath()
    );

    core.SetFrameCallback(
        [this](AppCapabilities& callbackCapabilities)
        {
            UpdateFrame(callbackCapabilities);
        });

    capabilities.logging->Info("App: Entering runtime loop. Close the window or press Escape to exit.");
    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void App::UpdateFrame(AppCapabilities& capabilities)
{
    capabilities.window->SetOverlayText(capabilities.window->BuildDebugViewText());
}
