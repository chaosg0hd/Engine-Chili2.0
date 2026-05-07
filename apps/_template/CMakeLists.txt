# Rename `engine_app_template` to your real executable target.
add_executable(engine_app_template
    src/main.cpp
    src/template_app.cpp
    src/template_app.hpp
    src/template_frame_builder.cpp
    src/template_frame_builder.hpp
    src/template_game.cpp
    src/template_game.hpp
)

target_include_directories(engine_app_template
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(engine_app_template
    PRIVATE
        engine_core
)

apply_project_warnings(engine_app_template)
enable_project_sanitizers(engine_app_template)

set_target_properties(engine_app_template PROPERTIES
    FOLDER "apps"
)
