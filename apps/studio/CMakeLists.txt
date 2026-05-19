# Studio is a host executable. It links EngineRuntime for editor systems, but
# game/app execution should move through dynamically loaded GameRuntime DLLs.
option(ENGINE_BUILD_STUDIO_SAMPLE_RUNTIMES "Build legacy sample runtimes into Studio for temporary in-process preview support." OFF)

# studio_core_lib: all studio infrastructure except the main entry point.
# Shared by Studio.exe (editor) and project game executables (e.g. pong.exe).
add_library(studio_core_lib STATIC
    src/studio_app.cpp
    src/studio_host.cpp
    src/bridge/engine_bridge.cpp
    src/commands/command_router.cpp
    src/runtime/runtime_commands.cpp
    src/runtime/scene_runtime.cpp
    src/runtime/scene_serializer.cpp
    src/runtime/runtime_queries.cpp
    src/runtime/runtime_world.cpp
    src/runtime/cursor_space_calibration.cpp
    src/runtime/proxy_prototype_resolver.cpp
    src/runtime/studio_default_scene_template.cpp
    src/runtime/default_scene_json.cpp
    src/runtime/vector_icon.cpp
    src/runtime/studio_camera.cpp
    src/runtime/studio_connector.cpp
    src/runtime/studio_interaction_controller.cpp
    src/runtime/studio_picking_service.cpp
    src/runtime/studio_preview_runtime.cpp
    src/runtime/runtime_registry.cpp
    src/runtime/studio_runtime_host.cpp
    src/transport/http_server.cpp
    src/studio/file_management_dialog.cpp
    src/studio/new_project_dialog.cpp
    src/studio/project_explorer_panel.cpp
    src/studio/studio_console_panel.cpp
    src/studio/studio_layout_state.cpp
    src/studio/studio_file_actions.cpp
    src/studio/studio_build_system.cpp
    src/studio/file_proxy.cpp
    src/studio/proxy_library.cpp
    src/studio/studio_project_system.cpp
    src/studio/studio_template_system.cpp
    src/studio_app.hpp
    src/studio_host.hpp
    src/bridge/engine_bridge.hpp
    src/commands/command_router.hpp
    src/runtime/runtime_commands.hpp
    src/runtime/scene_runtime.hpp
    src/runtime/scene_prototype.hpp
    src/runtime/scene_serializer.hpp
    src/runtime/runtime_queries.hpp
    src/runtime/runtime_world.hpp
    src/runtime/cursor_space_calibration.h
    src/runtime/proxy_prototype_resolver.hpp
    src/runtime/studio_default_scene_template.hpp
    src/runtime/default_scene_json.hpp
    src/runtime/vector_icon.hpp
    src/runtime/studio_camera.hpp
    src/runtime/studio_connector.hpp
    src/runtime/studio_interaction_controller.hpp
    src/runtime/studio_picking_service.hpp
    src/runtime/studio_preview_runtime.hpp
    src/runtime/runtime_game_api.hpp
    src/runtime/runtime_registry.hpp
    src/runtime/studio_runtime_host.hpp
    src/transport/http_server.hpp
    src/studio/file_management_dialog.hpp
    src/studio/new_project_dialog.hpp
    src/studio/project_explorer_panel.hpp
    src/studio/studio_console_panel.hpp
    src/studio/studio_layout_state.hpp
    src/studio/studio_file_actions.hpp
    src/studio/studio_build_system.hpp
    src/studio/file_proxy.hpp
    src/studio/proxy_library.hpp
    src/studio/studio_project_system.hpp
    src/studio/studio_template_system.hpp
    # Pong game scripts — compiled into studio_core_lib so both Studio.exe
    # (editor preview) and pong.exe (game build) get them via RegisterStudioBehaviors.
    ${PROJECT_SOURCE_DIR}/apps/pong/src/pong_rules.hpp
    ${PROJECT_SOURCE_DIR}/apps/pong/src/pong_ball_script.hpp
    ${PROJECT_SOURCE_DIR}/apps/pong/src/pong_ball_script.cpp
    ${PROJECT_SOURCE_DIR}/apps/pong/src/pong_paddle_script.hpp
    ${PROJECT_SOURCE_DIR}/apps/pong/src/pong_paddle_script.cpp
    ${PROJECT_SOURCE_DIR}/apps/pong/src/scene_manager_script.hpp
    ${PROJECT_SOURCE_DIR}/apps/pong/src/scene_manager_script.cpp
)

if(ENGINE_BUILD_STUDIO_SAMPLE_RUNTIMES)
    target_sources(studio_core_lib PRIVATE
        src/runtime/hello_game_runtime.cpp
        src/runtime/hello_game_runtime.hpp
    )
    target_compile_definitions(studio_core_lib PRIVATE ENGINE_STUDIO_ENABLE_SAMPLE_RUNTIMES=1)
endif()

target_include_directories(studio_core_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/apps/pong/src
)

target_link_libraries(studio_core_lib
    PUBLIC
        EngineRuntime
)

if(WIN32)
    target_link_libraries(studio_core_lib
        PUBLIC
            ws2_32
    )
endif()

apply_project_warnings(studio_core_lib)
enable_project_sanitizers(studio_core_lib)

set_target_properties(studio_core_lib PROPERTIES
    FOLDER "apps"
)

# Studio.exe: editor entry point links against studio_core_lib.
add_executable(Studio src/main.cpp)

target_link_libraries(Studio
    PRIVATE
        studio_core_lib
)

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
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        "$<TARGET_FILE_DIR:Studio>/coretools/runtime"
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
