add_executable(engine_sandbox
    src/main.cpp
    src/sandbox_app.cpp
    ${PROJECT_SOURCE_DIR}/src/app/app.cpp
    ${PROJECT_SOURCE_DIR}/src/app/app.hpp
)

target_include_directories(engine_sandbox
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(engine_sandbox
    PRIVATE
        engine_core
)

apply_project_warnings(engine_sandbox)
enable_project_sanitizers(engine_sandbox)

set_target_properties(engine_sandbox PROPERTIES
    FOLDER "apps"
)
