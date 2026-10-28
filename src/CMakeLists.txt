# Pure game logic. The tests link this library, so nothing in it may open a window.
add_library(horde_core STATIC
    core/Arena.cpp
    core/AttackChoice.cpp
    core/Boss.cpp
    core/Chase.cpp
    core/Collision.cpp
    core/Dash.cpp
    core/Facing.cpp
    core/FixedTimestep.cpp
    core/Flasks.cpp
    core/FollowCamera.cpp
    core/Health.cpp
    core/HitStop.cpp
    core/ImpactFeel.cpp
    core/Loadout.cpp
    core/Menu.cpp
    core/Movement.cpp
    core/Particle.cpp
    core/ParticleEmitter.cpp
    core/Projectile.cpp
    core/Random.cpp
    core/ScreenFit.cpp
    core/ScreenShake.cpp
    core/Session.cpp
    core/Settings.cpp
    core/Shield.cpp
    core/Shockwave.cpp
    core/SoundCue.cpp
    core/SoundLibrary.cpp
    core/SpawnMarker.cpp
    core/StoneFloor.cpp
    core/Synth.cpp
    core/TrailingBar.cpp
    core/Weapon.cpp
    core/World.cpp)
target_include_directories(horde_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
horde_set_warnings(horde_core)

# The game itself: window, input, rendering and the main loop.
add_executable(HordeCarnage
    main.cpp
    app/Game.cpp
    app/audio/SoundBank.cpp
    app/PlayerInput.cpp
    app/SettingsStore.cpp
    app/render/ArenaDraw.cpp
    app/render/Backdrop.cpp
    app/render/Art.cpp
    app/render/BossDraw.cpp
    app/render/DebugOverlay.cpp
    app/render/EndScreen.cpp
    app/render/HairBody.cpp
    app/render/Hud.cpp
    app/render/MenuDraw.cpp
    app/render/ParticleDraw.cpp
    app/render/PlayerDraw.cpp
    app/render/Shapes.cpp
    app/render/Text.cpp
    app/render/View.cpp
    app/render/WorldDraw.cpp)
target_link_libraries(HordeCarnage PRIVATE horde_core raylib)
target_compile_definitions(HordeCarnage PRIVATE "HORDE_VERSION=\"${PROJECT_VERSION}\"")
horde_set_warnings(HordeCarnage)

# The fonts are compiled into the executable (see cmake/EmbedFile.cmake), and their license is copied next to
# it, where the release zip and the web build pick it up.
horde_embed_file(HordeCarnage "${cinzel_regular_SOURCE_DIR}/Cinzel-Regular.ttf" kCinzelRegular)
horde_embed_file(HordeCarnage "${cinzel_bold_SOURCE_DIR}/Cinzel-Bold.ttf" kCinzelBold)
add_custom_command(TARGET HordeCarnage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cinzel_license_SOURCE_DIR}/OFL.txt"
            "$<TARGET_FILE_DIR:HordeCarnage>/Cinzel-OFL.txt")

# The browser build: index.html (from our page in web/), index.js and index.wasm, ready for itch.io.
# The memory grows if it has to, and GLFW comes from Emscripten's own port, which raylib's web platform needs.
# The stack is 1 MB instead of Emscripten's 64 KB: a restart builds a fresh World of about 20 KB on it, and a
# stack that overflows in WebAssembly does not crash at once but overwrites the program's globals.
if(EMSCRIPTEN)
    set_target_properties(HordeCarnage PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
    target_link_options(HordeCarnage PRIVATE
        -sUSE_GLFW=3
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=1MB
        --shell-file ${PROJECT_SOURCE_DIR}/web/shell.html)
    set_property(TARGET HordeCarnage APPEND PROPERTY LINK_DEPENDS ${PROJECT_SOURCE_DIR}/web/shell.html)
endif()

# Release builds are a plain windowed app. Debug keeps the console for raylib's log.
set_target_properties(HordeCarnage PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
if(MSVC)
    target_link_options(HordeCarnage PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HordeCarnage)
endif()
