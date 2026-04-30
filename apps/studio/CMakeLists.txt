# Studio is a host executable. It links EngineRuntime for editor systems, but
# game/app execution should move through dynamically loaded GameRuntime DLLs.
add_executable(Studio
    src/main.cpp
    src/studio_app.cpp
    src/studio_host.cpp
    src/bridge/engine_bridge.cpp
    src/commands/command_router.cpp
    src/runtime/hello_game_runtime.cpp
    src/runtime/runtime_registry.cpp
    src/runtime/studio_runtime_host.cpp
    src/transport/http_server.cpp
    src/studio/file_management_dialog.cpp
    src/studio/new_project_dialog.cpp
    src/studio/project_explorer_panel.cpp
    src/studio/studio_console_panel.cpp
    src/studio/studio_file_actions.cpp
    src/studio/studio_build_system.cpp
    src/studio/file_proxy.cpp
    src/studio/studio_project_system.cpp
    src/studio/studio_template_system.cpp
    src/studio_app.hpp
    src/studio_host.hpp
    src/bridge/engine_bridge.hpp
    src/commands/command_router.hpp
    src/runtime/hello_game_runtime.hpp
    src/runtime/runtime_game_api.hpp
    src/runtime/runtime_registry.hpp
    src/runtime/studio_runtime_host.hpp
    src/transport/http_server.hpp
    src/studio/file_management_dialog.hpp
    src/studio/new_project_dialog.hpp
    src/studio/project_explorer_panel.hpp
    src/studio/studio_console_panel.hpp
    src/studio/studio_file_actions.hpp
    src/studio/studio_build_system.hpp
    src/studio/file_proxy.hpp
    src/studio/studio_project_system.hpp
    src/studio/studio_template_system.hpp
)

target_include_directories(Studio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(Studio
    PRIVATE
        EngineRuntime
)

if(WIN32)
    target_link_libraries(Studio
        PRIVATE
            ws2_32
    )
endif()

set(_engine_studio_webview2_loader_hint
    "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/Markdown/runtimes/win-x64/native/WebView2Loader.dll")

if (EXISTS "${_engine_studio_webview2_loader_hint}")
    add_custom_command(TARGET Studio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_engine_studio_webview2_loader_hint}"
            "$<TARGET_FILE_DIR:Studio>/WebView2Loader.dll")
endif()

add_custom_command(TARGET Studio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:EngineRuntime>"
        "$<TARGET_FILE_DIR:Studio>/$<TARGET_FILE_NAME:EngineRuntime>"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Studio>/coretools/runtime"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/coretools/runtime"
        "$<TARGET_FILE_DIR:Studio>/coretools/runtime")

apply_project_warnings(Studio)
enable_project_sanitizers(Studio)

set_target_properties(Studio PROPERTIES
    FOLDER "apps"
    OUTPUT_NAME "engine_studio"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/studio"
)
