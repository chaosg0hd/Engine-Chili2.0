cmake_minimum_required(VERSION 3.20)

project(ProjectEngine
    VERSION 0.1.0
    DESCRIPTION "Modular C++ game engine sandbox and studio shell"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Output binaries to /build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(cmake/compiler_warnings.cmake)
include(cmake/sanitizers.cmake)

add_subdirectory(core)
add_subdirectory(apps/sandbox)
add_subdirectory(apps/studio)
