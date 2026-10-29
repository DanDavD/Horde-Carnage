#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>

#include "core/AnalogInput.h"
#include "core/GamepadAim.h"
#include "core/World.h"

namespace {

constexpr float kPi = 3.1415926535f;
constexpr float kDeadzone = 0.2f;
constexpr float kTolerance = 0.001f;
constexpr horde::Vec2 kView{1280.0f, 720.0f};

float AngleBetween(horde::Vec2 a, horde::Vec2 b) {
    const float dot = horde::Dot(horde::Normalized(a), horde::Normalized(b));
    return std::acos(std::clamp(dot, -1.0f, 1.0f));
}

} // namespace

TEST_CASE("gamepad right stick deadzone suppresses small resting drift") {
    const horde::Vec2 drift{0.10f, -0.11f};
    CHECK(horde::ApplyRadialDeadzone(drift, kDeadzone) == horde::Vec2{});

    const horde::Vec2 pushed{0.6f, 0.0f};
    const horde::Vec2 active = horde::ApplyRadialDeadzone(pushed, kDeadzone);
    CHECK(active.x > 0.0f);
    CHECK(active.y == 0.0f);
}

TEST_CASE("gamepad aim point places crosshair at fixed distance") {
    const horde::Vec2 player{100.0f, 200.0f};

    SUBCASE("active stick aim") {
        const horde::Vec2 stick{1.0f, 0.0f};
        const horde::Vec2 fallback{0.0f, -1.0f};
        const horde::Vec2 aim = horde::GamepadAimPoint(player, stick, fallback, 260.0f);
        CHECK(doctest::Approx(aim.x).epsilon(kTolerance) == 360.0f);
        CHECK(doctest::Approx(aim.y).epsilon(kTolerance) == 200.0f);
    }

    SUBCASE("zero stick uses fallback direction") {
        const horde::Vec2 stick{};
        const horde::Vec2 fallback{0.0f, 1.0f};
        const horde::Vec2 aim = horde::GamepadAimPoint(player, stick, fallback, 260.0f);
        CHECK(doctest::Approx(aim.x).epsilon(kTolerance) == 100.0f);
        CHECK(doctest::Approx(aim.y).epsilon(kTolerance) == 460.0f);
    }

    SUBCASE("zero stick and zero fallback defaults to facing up") {
        const horde::Vec2 aim = horde::GamepadAimPoint(player, {}, {}, 260.0f);
        CHECK(doctest::Approx(aim.x).epsilon(kTolerance) == 100.0f);
        CHECK(doctest::Approx(aim.y).epsilon(kTolerance) == -60.0f);
    }
}

TEST_CASE("aim assist gently bends trajectory toward valid target in cone") {
    const horde::Vec2 player{0.0f, 0.0f};
    const horde::Vec2 aimRight{1.0f, 0.0f};

    SUBCASE("no targets leaves aim unchanged") {
        const horde::Vec2 result = horde::ApplyAimAssist(player, aimRight, {});
        CHECK(doctest::Approx(result.x).epsilon(kTolerance) == 1.0f);
        CHECK(doctest::Approx(result.y).epsilon(kTolerance) == 0.0f);
    }

    SUBCASE("target directly in line of sight stays unchanged") {
        const horde::Vec2 targets[] = {{300.0f, 0.0f}};
        const horde::Vec2 result = horde::ApplyAimAssist(player, aimRight, targets);
        CHECK(doctest::Approx(result.x).epsilon(kTolerance) == 1.0f);
        CHECK(doctest::Approx(result.y).epsilon(kTolerance) == 0.0f);
    }

    SUBCASE("target at 10 degrees is softly pulled closer") {
        const float angleRad = 10.0f * (kPi / 180.0f);
        const horde::Vec2 target{std::cos(angleRad) * 350.0f, std::sin(angleRad) * 350.0f};
        const horde::Vec2 targets[] = {target};

        const horde::Vec2 assisted = horde::ApplyAimAssist(player, aimRight, targets);
        const float originalDiff = AngleBetween(aimRight, target);
        const float assistedDiff = AngleBetween(assisted, target);

        // Closer to the target than the raw stick, without snapping straight onto it.
        CHECK(assistedDiff < originalDiff);
        CHECK(assistedDiff > 0.01f);
        CHECK(assisted.y > 0.0f);
    }

    SUBCASE("target outside the assist cone receives no pull") {
        const float angleRad = 35.0f * (kPi / 180.0f);
        const horde::Vec2 target{std::cos(angleRad) * 300.0f, std::sin(angleRad) * 300.0f};
        const horde::Vec2 targets[] = {target};

        const horde::Vec2 assisted = horde::ApplyAimAssist(player, aimRight, targets);
        CHECK(doctest::Approx(assisted.x).epsilon(kTolerance) == 1.0f);
        CHECK(doctest::Approx(assisted.y).epsilon(kTolerance) == 0.0f);
    }

    SUBCASE("target beyond max range receives no pull") {
        const horde::Vec2 targets[] = {{950.0f, 50.0f}};
        const horde::Vec2 assisted = horde::ApplyAimAssist(player, aimRight, targets);
        CHECK(doctest::Approx(assisted.x).epsilon(kTolerance) == 1.0f);
        CHECK(doctest::Approx(assisted.y).epsilon(kTolerance) == 0.0f);
    }

    SUBCASE("multiple targets picks the one closest to line of sight") {
        const float radA = 18.0f * (kPi / 180.0f);
        const float radB = 6.0f * (kPi / 180.0f);
        const horde::Vec2 targets[] = {
            {std::cos(radA) * 300.0f, std::sin(radA) * 300.0f},
            {std::cos(radB) * 300.0f, std::sin(radB) * 300.0f},
        };

        const horde::Vec2 assisted = horde::ApplyAimAssist(player, aimRight, targets);
        const float angleAssisted = std::atan2(assisted.y, assisted.x);
        CHECK(angleAssisted > 0.0f);
        CHECK(angleAssisted < radB);
    }
}

TEST_CASE("assist aim in world targets the boss and minions while alive") {
    horde::World world = horde::CreateWorld(horde::kBossArena, kView, 123);
    const horde::Vec2 playerPos = world.player.motion.position;
    const horde::Vec2 toBoss = horde::Normalized(world.boss.motion.position - playerPos);
    CHECK(doctest::Approx(toBoss.x).epsilon(kTolerance) == 0.0f);
    CHECK(doctest::Approx(toBoss.y).epsilon(kTolerance) == -1.0f);

    SUBCASE("aiming slightly off from the living boss pulls toward her") {
        const float offAngle = 8.0f * (kPi / 180.0f);
        const horde::Vec2 rawAim{std::sin(offAngle), -std::cos(offAngle)};

        const horde::Vec2 assisted = horde::AssistAimInWorld(world, rawAim);
        const float rawDiff = AngleBetween(rawAim, toBoss);
        const float assistedDiff = AngleBetween(assisted, toBoss);
        CHECK(assistedDiff < rawDiff);
    }

    SUBCASE("defeated boss is not targeted by aim assist") {
        horde::DefeatBoss(world);
        const float offAngle = 8.0f * (kPi / 180.0f);
        const horde::Vec2 rawAim{std::sin(offAngle), -std::cos(offAngle)};

        const horde::Vec2 assisted = horde::AssistAimInWorld(world, rawAim);
        CHECK(doctest::Approx(assisted.x).epsilon(kTolerance) == rawAim.x);
        CHECK(doctest::Approx(assisted.y).epsilon(kTolerance) == rawAim.y);
    }

    SUBCASE("living minions in the enemies pool are targeted") {
        const horde::Vec2 minionPos = playerPos + horde::Vec2{300.0f, 0.0f};
        horde::Enemy* enemy = world.enemies.Spawn(horde::Enemy{.motion = horde::MotionAt(minionPos)});
        REQUIRE(enemy != nullptr);
        enemy->health.current = 20;

        const float offAngle = 8.0f * (kPi / 180.0f);
        const horde::Vec2 rawAim{std::cos(offAngle), std::sin(offAngle)};

        const horde::Vec2 assisted = horde::AssistAimInWorld(world, rawAim);
        const horde::Vec2 toMinion{1.0f, 0.0f};
        CHECK(AngleBetween(assisted, toMinion) < AngleBetween(rawAim, toMinion));
    }
}

TEST_CASE("only the right stick aims: the left stick never turns the aim, firing or not") {
    const horde::World world = horde::CreateWorld(horde::kBossArena, kView, 456);
    const horde::Vec2 playerPos = world.player.motion.position;
    horde::GamepadAim aim;

    // Right stick north: aims north at the boss.
    const horde::Vec2 north = horde::StepGamepadAim(aim, world, {0.0f, -1.0f}, true);
    CHECK(doctest::Approx(north.x).epsilon(kTolerance) == playerPos.x);
    CHECK(doctest::Approx(north.y).epsilon(kTolerance) == playerPos.y - horde::kGamepadCrosshairDistance);

    // Letting go of the right stick, firing or not, keeps aiming north however the hunter walks: the left
    // stick is not even an input of the aim.
    const horde::Vec2 held = horde::StepGamepadAim(aim, world, {}, true);
    CHECK(doctest::Approx(held.y).epsilon(kTolerance) == playerPos.y - horde::kGamepadCrosshairDistance);
    const horde::Vec2 idle = horde::StepGamepadAim(aim, world, {}, false);
    CHECK(doctest::Approx(idle.x).epsilon(kTolerance) == north.x);
    CHECK(doctest::Approx(idle.y).epsilon(kTolerance) == north.y);
}

TEST_CASE("holding fire with right stick released does not compound aim assist across steps") {
    const horde::World world = horde::CreateWorld(horde::kBossArena, kView, 789);
    horde::GamepadAim aim;
    const float offAngle = 10.0f * (kPi / 180.0f);
    const horde::Vec2 stick{std::sin(offAngle), -std::cos(offAngle)};

    (void)horde::StepGamepadAim(aim, world, stick, true);
    const horde::Vec2 initialAssisted = aim.direction;

    for (int i = 0; i < 120; ++i) {
        (void)horde::StepGamepadAim(aim, world, {}, true);
    }
    CHECK(doctest::Approx(aim.direction.x).epsilon(kTolerance) == initialAssisted.x);
    CHECK(doctest::Approx(aim.direction.y).epsilon(kTolerance) == initialAssisted.y);
}
