#pragma once

#include "prototypes/presentation/frame.hpp"

#include <string>

namespace studio_runtime
{
    struct ProjectRuntimeDesc
    {
        std::string projectId;
        std::string projectName;
        std::string projectRootPath;
        std::string runtimeName;
        std::string defaultScenePath;
        std::string sourceEntryPath;
    };

    struct RuntimeInput
    {
        bool escapePressed = false;
        bool servePressed = false;
        bool resetPressed = false;
        bool leftUpDown = false;
        bool leftDownDown = false;
        bool rightUpDown = false;
        bool rightDownDown = false;
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
