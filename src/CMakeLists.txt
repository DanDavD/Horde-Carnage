# Pure game logic. The tests link this library, so nothing in it may open a window.
add_library(horde_core STATIC
    core/Arena.cpp
    core/Chase.cpp
    core/Collision.cpp
    core/Facing.cpp
    core/FixedTimestep.cpp
    core/FollowCamera.cpp
    core/Health.cpp
    core/Loadout.cpp
    core/Movement.cpp
    core/Projectile.cpp
    core/Session.cpp
    core/Weapon.cpp
    core/World.cpp)
target_include_directories(horde_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
horde_set_warnings(horde_core)

# The game itself: window, input, rendering and the main loop.
add_executable(HordeCarnage
    main.cpp
    app/Game.cpp
    app/PlayerInput.cpp)
target_link_libraries(HordeCarnage PRIVATE horde_core raylib)
target_compile_definitions(HordeCarnage PRIVATE "HORDE_VERSION=\"${PROJECT_VERSION}\"")
horde_set_warnings(HordeCarnage)

# Release builds are a plain windowed app. Debug keeps the console for raylib's log.
set_target_properties(HordeCarnage PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
if(MSVC)
    target_link_options(HordeCarnage PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HordeCarnage)
endif()
