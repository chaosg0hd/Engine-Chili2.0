#pragma once

#include "prototypes/presentation/frame.hpp"
#include "input/raw_input_state.h"

#include <string>

namespace studio_runtime
{
    struct ProjectRuntimeDesc
    {
        std::string projectId;
        std::string projectName;
        std::string projectRootPath;

        // TEMPORARY: in-process preview runtime class name, looked up from RuntimeRegistry.
        // Long-term owner: project build system via exported artifact DLL (see exportedArtifactPath).
        // Deferral reason: DLL hot-load not yet implemented (binary shape: launcher→engine.dll→app DLL).
        // Must not be used as a persistent project identity — it is an editor preview shortcut only.
        std::string previewRuntimeName;

        // Reserved: absolute path to the project-built runtime DLL artifact.
        // When set, StudioRuntimeHost::Play launches the artifact instead of an in-process preview class.
        // Currently unused — will replace previewRuntimeName once the DLL stack is live.
        std::string exportedArtifactPath;

        std::string defaultScenePath;
        std::string sourceEntryPath;
    };

    struct RuntimeInput
    {
        struct CursorBindingDebugData
        {
            bool valid = false;
            int osScreenX = 0;
            int osScreenY = 0;
            int clientX = 0;
            int clientY = 0;
            int clientDpiAdjustedX = 0;
            int clientDpiAdjustedY = 0;
            int windowScreenLeft = 0;
            int windowScreenTop = 0;
            int windowScreenRight = 0;
            int windowScreenBottom = 0;
            int windowWidth = 0;
            int windowHeight = 0;
            int clientWidth = 0;
            int clientHeight = 0;
            int borderLeft = 0;
            int borderTop = 0;
            int borderRight = 0;
            int borderBottom = 0;
            int viewportInsetLeft = 0;
            int viewportInsetTop = 0;
            int viewportInsetRight = 0;
            int viewportInsetBottom = 0;
            int viewportLocalX = 0;
            int viewportLocalY = 0;
            float viewportU = 0.0f;
            float viewportV = 0.0f;
            int renderX = 0;
            int renderY = 0;
            int viewportLocalDpiAdjustedX = 0;
            int viewportLocalDpiAdjustedY = 0;
            int drawX = 0;
            int drawY = 0;
            int drawDpiAdjustedX = 0;
            int drawDpiAdjustedY = 0;
            int calibrationDrawX = 0;
            int calibrationDrawY = 0;
            float dpiScaleX = 1.0f;
            float dpiScaleY = 1.0f;
        };

        bool escapePressed = false;
        bool servePressed = false;
        bool resetPressed = false;
        bool leftUpDown = false;
        bool leftDownDown = false;
        bool rightUpDown = false;
        bool rightDownDown = false;
        bool leftMousePressed = false;
        bool middleMouseDown = false;
        bool rightMouseDown = false;
        int mouseX = 0;
        int mouseY = 0;
        int mouseDeltaX = 0;
        int mouseDeltaY = 0;
        int mouseScrollDelta = 0;
        RawInputState rawInput;
        CursorBindingDebugData cursorDebug;
    };

    struct RuntimeFrame
    {
        std::string textOutput;
        FramePrototype renderFrame;
        bool hasRenderFrame = false;
        bool exitRequested = false;
    };

    class IGameRuntime
    {
    public:
        virtual ~IGameRuntime() = default;

        virtual void BeginPlay(const ProjectRuntimeDesc& project) = 0;
        virtual void EndPlay() = 0;
        virtual void Tick(float deltaSeconds, const RuntimeInput& input, RuntimeFrame& frame) = 0;
    };
}
