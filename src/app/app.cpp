#include "app.hpp"

#include "../core/engine_core.hpp"

#include <string>

bool App::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    core.LogInfo(
        std::string("App: Generic runner ready | file logging = ") +
        (core.IsFileLoggingAvailable() ? "true" : "false") +
        " | log path = " +
        core.GetLogFilePath()
    );

    core.SetFrameCallback(
        [this](EngineCore& callbackCore)
        {
            UpdateFrame(callbackCore);
        });

    core.LogInfo("App: Entering runtime loop. Close the window or press Escape to exit.");
    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void App::UpdateFrame(EngineCore& core)
{
    core.SetWindowOverlayText(core.BuildDebugViewText());
}
