# HotBuildTool is an external helper executable. It must not become an engine
# module; later it will build GameRuntime DLLs and copy versioned hot-load files.
add_executable(HotBuildTool
    src/main.cpp
)

apply_project_warnings(HotBuildTool)
enable_project_sanitizers(HotBuildTool)

set_target_properties(HotBuildTool PROPERTIES
    FOLDER "tools"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tools/hotbuild"
)
