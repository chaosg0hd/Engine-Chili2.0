add_executable(engine_studio
    src/main.cpp
    src/studio_app.cpp
    src/studio_host.cpp
    src/bridge/engine_bridge.cpp
    src/events/event_bus.cpp
    src/network/http_server.cpp
    src/network/websocket_server.cpp
    src/commands/command_router.cpp
    src/studio_app.hpp
    src/studio_host.hpp
    src/bridge/engine_bridge.hpp
    src/events/event_bus.hpp
    src/network/http_server.hpp
    src/network/websocket_server.hpp
    src/commands/command_router.hpp
)

target_include_directories(engine_studio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(engine_studio
    PRIVATE
        engine_core
        ws2_32
)

apply_project_warnings(engine_studio)
enable_project_sanitizers(engine_studio)

set_target_properties(engine_studio PROPERTIES
    FOLDER "apps"
)
