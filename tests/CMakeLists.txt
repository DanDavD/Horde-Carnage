# Unit tests for the pure logic in horde_core. They never open a window, so they also run in CI.
add_executable(horde_tests
    TestMain.cpp
    core/AnalogInputTests.cpp
    core/ArenaTests.cpp
    core/BossTests.cpp
    core/ChaseTests.cpp
    core/CollisionTests.cpp
    core/DashTests.cpp
    core/FacingTests.cpp
    core/FixedTimestepTests.cpp
    core/FlasksTests.cpp
    core/FollowCameraTests.cpp
    core/HealthTests.cpp
    core/LoadoutTests.cpp
    core/MovementTests.cpp
    core/ObjectPoolTests.cpp
    core/ProjectileTests.cpp
    core/SessionTests.cpp
    core/Vec2Tests.cpp
    core/WeaponTests.cpp
    core/WorldTests.cpp)
target_link_libraries(horde_tests PRIVATE horde_core doctest::doctest)
horde_set_warnings(horde_tests)

add_test(NAME horde_tests COMMAND horde_tests)
