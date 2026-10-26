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

# Cinzel (SIL Open Font License 1.1): the serif of the titles and the HUD, Regular and Bold. Single files from
# the font's own repository, pinned to one commit and checked against their hashes. The license travels with
# the game: the build copies it next to the executable.
set(HORDE_CINZEL_URL https://raw.githubusercontent.com/NDISCOVER/Cinzel/dd598495b0fb2ad84270d5cc75d642d2f1e8eabf)
FetchContent_Declare(cinzel_regular
    URL ${HORDE_CINZEL_URL}/fonts/ttf/Cinzel-Regular.ttf
    URL_HASH SHA256=af0031129f27dc752e8629a80b793d27abea94027faa27cc660c3fc33f607a1f
    DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_Declare(cinzel_bold
    URL ${HORDE_CINZEL_URL}/fonts/ttf/Cinzel-Bold.ttf
    URL_HASH SHA256=0c23ec565db45c5508ee95889c60ad87debd167ca07167a43a5d68572b4e2eac
    DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_Declare(cinzel_license
    URL ${HORDE_CINZEL_URL}/OFL.txt
    URL_HASH SHA256=a46624198eeb4c2e442c38b0ff3bd8f52caeefd76675c36f410e2fb69014a239
    DOWNLOAD_NO_EXTRACT TRUE)

FetchContent_MakeAvailable(raylib doctest cinzel_regular cinzel_bold cinzel_license)
