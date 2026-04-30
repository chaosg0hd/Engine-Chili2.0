# Pong owns its app/game runtime DLL. Hosts load this target through the narrow
# C API instead of compiling Pong gameplay directly.
add_library(PongRuntime SHARED
    runtime/game_runtime_api.cpp
    src/pong_app.cpp
    src/pong_app.hpp
    src/pong_frame_builder.cpp
    src/pong_frame_builder.hpp
    src/pong_game.cpp
    src/pong_game.hpp
)

target_include_directories(PongRuntime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PongRuntime
    PRIVATE
        EngineRuntime
)

target_compile_definitions(PongRuntime
    PRIVATE
        GAME_RUNTIME_EXPORTS
)

apply_project_warnings(PongRuntime)
enable_project_sanitizers(PongRuntime)

set_target_properties(PongRuntime PROPERTIES
    FOLDER "apps/pong"
    OUTPUT_NAME "pong_runtime"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/apps/pong"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/apps/pong"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/apps/pong"
)

# Pong's preview host is intentionally tiny: it loads the Pong GameRuntime DLL
# and hands control through get_game_api().
add_executable(PongPreview
    preview/main.cpp
)

target_link_libraries(PongPreview
    PRIVATE
        EngineRuntime
)

apply_project_warnings(PongPreview)
enable_project_sanitizers(PongPreview)

if(WIN32)
    add_custom_command(TARGET PongPreview POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:EngineRuntime>"
            "$<TARGET_FILE_DIR:PongPreview>/$<TARGET_FILE_NAME:EngineRuntime>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:PongRuntime>"
            "$<TARGET_FILE_DIR:PongPreview>/$<TARGET_FILE_NAME:PongRuntime>")
endif()

set_target_properties(PongPreview PROPERTIES
    FOLDER "apps/pong"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/apps/pong"
)
