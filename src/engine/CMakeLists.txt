# EngineRuntime is the reusable engine DLL. It owns engine systems and module
# coordination; game/editor behavior must live in higher-level DLLs or hosts.
file(GLOB_RECURSE ENGINE_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/app/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/input/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/modules/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/native_ui/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/prototypes/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/systems/*.cpp"
)

file(GLOB_RECURSE ENGINE_RUNTIME_HEADERS CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/app/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/core/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/input/*.h"
    "${PROJECT_SOURCE_DIR}/src/modules/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/native_ui/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/prototypes/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/systems/*.hpp"
)

add_library(EngineRuntime SHARED
    ${ENGINE_RUNTIME_SOURCES}
    ${ENGINE_RUNTIME_HEADERS}
)

add_library(engine_core ALIAS EngineRuntime)

target_include_directories(EngineRuntime
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_features(EngineRuntime PUBLIC cxx_std_17)
apply_project_warnings(EngineRuntime)
enable_project_sanitizers(EngineRuntime)

if(WIN32)
    target_link_libraries(EngineRuntime
        PUBLIC
            ole32
            d3d11
            d3dcompiler
            dxgi
    )
endif()

if(MINGW)
    target_link_libraries(EngineRuntime
        PUBLIC
            gdi32
            winmm
            xaudio2_9
    )
endif()

set_target_properties(EngineRuntime PROPERTIES
    FOLDER "runtime"
    OUTPUT_NAME "engine"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/engine"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/engine"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/engine"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
