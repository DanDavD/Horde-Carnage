#include <doctest/doctest.h>

#include "core/Boss.h"
#include "core/Dash.h"
#include "core/Flasks.h"
#include "core/LaserSight.h"
#include "core/Loadout.h"
#include "core/Weapon.h"
#include "core/World.h"

namespace {

constexpr horde::Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

} // namespace

TEST_CASE("hunter projects a laser sight only while alive with hands free") {
    horde::Player player;
    player.health.current = 60;
    CHECK(horde::CanProjectLaserSight(player));

    horde::Player dashing = player;
    REQUIRE(horde::TryStartDash(dashing.dash, dashing.motion.position, horde::Vec2{1.0f, 0.0f}));
    CHECK_FALSE(horde::CanProjectLaserSight(dashing));

    horde::Player drinking = player;
    REQUIRE(horde::TryStartDrinking(drinking.flasks));
    CHECK_FALSE(horde::CanProjectLaserSight(drinking));

    horde::Player dead = player;
    dead.health.current = 0;
    CHECK_FALSE(horde::CanProjectLaserSight(dead));
}

TEST_CASE("laser sight caps at the equipped gun's range in open space") {
    horde::World world = horde::CreateWorld(horde::kBossArena, kView);
    world.boss.health.current = 0;
    world.player.motion = horde::MotionAt(horde::Vec2{100.0f, 520.0f});
    world.player.facing = horde::Vec2{1.0f, 0.0f};

    const auto pistolSight = horde::TraceWorldLaserSight(world);
    REQUIRE(pistolSight.has_value());
    CHECK_FALSE(pistolSight->hitsTarget);
    CHECK_FALSE(pistolSight->hitsWall);
    CHECK(horde::Length(pistolSight->end - pistolSight->start) == doctest::Approx(horde::kPistol.range));

    horde::SelectSlot(world.player.loadout, 1);
    const auto smgSight = horde::TraceWorldLaserSight(world);
    REQUIRE(smgSight.has_value());
    CHECK_FALSE(smgSight->hitsTarget);
    CHECK_FALSE(smgSight->hitsWall);
    CHECK(horde::Length(smgSight->end - smgSight->start) == doctest::Approx(horde::kSmg.range));
}

TEST_CASE("laser sight clips against all four arena walls and handles a barrel pressed into a wall") {
    const horde::Arena arena = horde::kBossArena;

    const horde::LaserSight north =
        horde::TraceLaserSight(horde::Vec2{500.0f, 120.0f}, horde::Vec2{0.0f, -1.0f}, 750.0f, arena, {});
    CHECK(north.hitsWall);
    CHECK_FALSE(north.hitsTarget);
    CHECK(north.end.y == doctest::Approx(arena.min.y));

    const horde::LaserSight south =
        horde::TraceLaserSight(horde::Vec2{500.0f, 900.0f}, horde::Vec2{0.0f, 1.0f}, 750.0f, arena, {});
    CHECK(south.hitsWall);
    CHECK(south.end.y == doctest::Approx(arena.max.y));

    const horde::LaserSight east =
        horde::TraceLaserSight(horde::Vec2{1500.0f, 500.0f}, horde::Vec2{1.0f, 0.0f}, 750.0f, arena, {});
    CHECK(east.hitsWall);
    CHECK(east.end.x == doctest::Approx(arena.max.x));

    const horde::LaserSight west =
        horde::TraceLaserSight(horde::Vec2{140.0f, 500.0f}, horde::Vec2{-1.0f, 0.0f}, 750.0f, arena, {});
    CHECK(west.hitsWall);
    CHECK(west.end.x == doctest::Approx(arena.min.x));

    // Barrel poking past the east wall stops immediately at the muzzle instead of projecting backward.
    const horde::Vec2 pastWall{arena.max.x + 12.0f, 500.0f};
    const horde::LaserSight flush = horde::TraceLaserSight(pastWall, horde::Vec2{1.0f, 0.0f}, 750.0f, arena, {});
    CHECK(flush.hitsWall);
    CHECK(flush.end == pastWall);
}

TEST_CASE("laser sight stops at the nearest living body and passes through dead or airborne ones") {
    horde::World world = horde::CreateWorld(horde::kBossArena, kView);

    // At the start of a fight the hunter aims north at the Matriarch 400 units away: hits her south edge.
    const auto onBoss = horde::TraceWorldLaserSight(world);
    REQUIRE(onBoss.has_value());
    CHECK(onBoss->hitsTarget);
    CHECK_FALSE(onBoss->hitsWall);
    CHECK(onBoss->end.x == doctest::Approx(world.boss.motion.position.x));
    CHECK(onBoss->end.y == doctest::Approx(world.boss.motion.position.y + world.boss.radius));

    // A living hairball in front of the boss blocks the beam first; once dead, the beam reaches the boss again.
    horde::Enemy* blocker = world.enemies.Spawn(
        horde::Enemy{.motion = horde::MotionAt(world.player.motion.position + horde::Vec2{0.0f, -180.0f})});
    REQUIRE(blocker != nullptr);
    const auto onHairball = horde::TraceWorldLaserSight(world);
    REQUIRE(onHairball.has_value());
    CHECK(onHairball->hitsTarget);
    CHECK(onHairball->end.y == doctest::Approx(blocker->motion.position.y + blocker->radius));

    blocker->health.current = 0;
    const auto pastDeadHairball = horde::TraceWorldLaserSight(world);
    REQUIRE(pastDeadHairball.has_value());
    CHECK(pastDeadHairball->end.y == doctest::Approx(world.boss.motion.position.y + world.boss.radius));

    // While the boss is in the air during a Leap, the beam shines across the floor under her.
    world.enemies.Clear();
    world.boss.attack = horde::BossAttack{.kind = horde::AttackKind::Leap,
                                          .stage = horde::AttackStage::Warning,
                                          .stageSeconds = world.boss.leap.crouchSeconds + 0.2f,
                                          .takeoff = world.boss.motion.position,
                                          .target = world.boss.motion.position};
    REQUIRE(horde::IsAirborne(world.boss));
    const auto underLeap = horde::TraceWorldLaserSight(world);
    REQUIRE(underLeap.has_value());
    CHECK_FALSE(underLeap->hitsTarget);

    // Live frame aimDirection overrides player.facing without mutating the simulation state.
    const horde::Vec2 eastAim{1.0f, 0.0f};
    const auto aimedEast = horde::TraceWorldLaserSight(world, 1.0f, eastAim);
    REQUIRE(aimedEast.has_value());
    CHECK(aimedEast->end.x > aimedEast->start.x);
    CHECK(aimedEast->end.y == doctest::Approx(world.player.motion.position.y));
    CHECK(world.player.facing == horde::Vec2{0.0f, -1.0f});
    (void)kDt;
}
