# pong.exe: Studio runtime host with editor UI disabled, auto-starts in Play mode.
# Links studio_core_lib so it gets the engine, all runtime systems, and pong scripts.
add_executable(Pong
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_host.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_host.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene_manager.hpp
)

target_link_libraries(Pong
    PRIVATE
        studio_core_lib
)

# Copy scenes and runtime next to the exe.
add_custom_command(TARGET Pong POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/User/pong/Project/scenes/scene0.scene.json"
        "$<TARGET_FILE_DIR:Pong>/scene0.scene.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/User/pong/Project/scenes/pong.scene.json"
        "$<TARGET_FILE_DIR:Pong>/scene1.scene.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:EngineRuntime>"
        "$<TARGET_FILE_DIR:Pong>/$<TARGET_FILE_NAME:EngineRuntime>"
)

apply_project_warnings(Pong)
enable_project_sanitizers(Pong)

set_target_properties(Pong PROPERTIES
    FOLDER "apps"
    OUTPUT_NAME "pong"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/pong"
)
