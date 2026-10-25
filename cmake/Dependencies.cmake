# Third-party code is downloaded at configure time and pinned to a release,
# so every machine (and CI) builds against exactly the same version.
include(FetchContent)

# Some dependencies still declare a very old cmake_minimum_required, which CMake 4 rejects.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

# raylib: window, input, 2D rendering and audio. SYSTEM keeps its headers out of our strict warnings.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
# With the Emscripten toolchain, raylib builds for the browser: WebGL and the browser's own input and audio.
if(EMSCRIPTEN)
    set(PLATFORM "Web" CACHE STRING "" FORCE)
endif()
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SYSTEM)

# doctest: a small, header-only unit test framework that compiles fast.
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(doctest
    URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.11.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SYSTEM)

FetchContent_MakeAvailable(raylib doctest)
