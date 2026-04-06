cmake_minimum_required(VERSION 3.20)

project(Engine
    VERSION 0.1.0
    DESCRIPTION "Modular C++ game engine sandbox"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output binaries to /build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Gather all source files under src/
file(GLOB_RECURSE ENGINE_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Gather all headers too, mostly for IDE visibility
file(GLOB_RECURSE ENGINE_HEADERS
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
)

add_executable(engine
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
)

target_include_directories(engine PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

if(MINGW)
    target_link_libraries(engine PRIVATE
        gdi32
        winmm
    )
endif()

if(MSVC)
    target_compile_options(engine PRIVATE /W4)
else()
    target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic)
endif()