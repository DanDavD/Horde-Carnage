# Unit tests for the pure logic in horde_core. They never open a window, so they also run in CI.
add_executable(horde_tests
    TestMain.cpp
    core/FixedTimestepTests.cpp
    core/MovementTests.cpp
    core/Vec2Tests.cpp)
target_link_libraries(horde_tests PRIVATE horde_core doctest::doctest)
horde_set_warnings(horde_tests)

add_test(NAME horde_tests COMMAND horde_tests)
