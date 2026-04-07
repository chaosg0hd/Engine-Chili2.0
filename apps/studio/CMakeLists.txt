add_executable(engine_studio
    src/main.cpp
    src/studio_app.cpp
    src/studio_host.cpp
    src/bridge/engine_bridge.cpp
    src/commands/command_router.cpp
    src/layout/dock_layout.cpp
    src/webview/coretools_host.cpp
    src/studio_app.hpp
    src/studio_host.hpp
    src/bridge/engine_bridge.hpp
    src/commands/command_router.hpp
    src/layout/dock_layout.hpp
    src/webview/coretools_host.hpp
)

target_include_directories(engine_studio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(engine_studio
    PRIVATE
        engine_core
        ole32
)

set(_engine_studio_webview2_loader_hint
    "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/Markdown/runtimes/win-x64/native/WebView2Loader.dll")

if (EXISTS "${_engine_studio_webview2_loader_hint}")
    add_custom_command(TARGET engine_studio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_engine_studio_webview2_loader_hint}"
            "$<TARGET_FILE_DIR:engine_studio>/WebView2Loader.dll")
endif()

apply_project_warnings(engine_studio)
enable_project_sanitizers(engine_studio)

set_target_properties(engine_studio PROPERTIES
    FOLDER "apps"
)
