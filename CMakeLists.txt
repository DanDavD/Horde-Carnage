cmake_minimum_required(VERSION 3.25)

# The version here is the game version. The release steps in CONTRIBUTING.md bump it.
project(HordeCarnage VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The C++ runtime is linked into the executable, so the release zip runs on a PC that does not have the
# Visual C++ Redistributable installed. Set before the dependencies, so raylib and doctest use it too.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(HORDE_BUILD_TESTS "Build the unit tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Dependencies)
include(Warnings)

add_subdirectory(src)

if(HORDE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
