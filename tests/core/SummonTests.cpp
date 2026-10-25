#include <algorithm>
#include <array>
#include <numbers>

#include <doctest/doctest.h>

#include "PredictableBoss.h"
#include "core/Boss.h"
#include "core/SpawnMarker.h"
#include "core/World.h"

using horde::Arena;
using horde::AttackKind;
using horde::AttackStage;
using horde::Boss;
using horde::BossAttack;
using horde::BossPhase;
using horde::Enemy;
using horde::PlayerCommands;
using horde::SummonPlacement;
using horde::SummonPoints;
using horde::Vec2;
using horde::World;

namespace {

constexpr Arena kArena = horde::kBossArena;
constexpr Vec2 kCenter{820.0f, 520.0f};
constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;
// Where the boss stands in the tests that only look at the walls: far from every point they plan.
constexpr Vec2 kFarBoss{1500.0f, 950.0f};

bool InsideWalls(Vec2 point, float margin) {
    return point.x >= kArena.min.x + margin - 0.01f && point.x <= kArena.max.x - margin + 0.01f &&
           point.y >= kArena.min.y + margin - 0.01f && point.y <= kArena.max.y - margin + 0.01f;
}

// A fight in the second phase, the boss without its chance, in the middle and about to summon, with the
// player standing `offset` away. 500 units east, every mark of a summon is clear of the walls and of the boss,
// whatever its turn.
World AboutToSummon(Vec2 offset) {
    World world = horde::CreateWorld(kArena, kView);
    world.boss = horde::test::Predictable(world.boss);
    world.boss.phase = BossPhase::Two;
    world.boss.stats.maxSpeed = 0.0f;
    world.boss.attack = BossAttack{.kind = AttackKind::Summon};
    world.player.motion = horde::MotionAt(kCenter + offset);
    return world;
}

void Step(World& world, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepWorld(world, PlayerCommands{}, kDt);
    }
}

} // namespace

TEST_CASE("in the open, a summon's points ring the player at its distance, evenly spaced") {
    const SummonPlacement placement;
    const SummonPoints plan = horde::PlanSummon(kCenter, kFarBoss, kArena, placement, 0.0f);
    REQUIRE(plan.count == 4);
    for (std::size_t i = 0; i < plan.count; ++i) {
        CHECK(horde::Length(plan.points[i] - kCenter) == doctest::Approx(placement.distance));
    }
    // Turned by 0: east, south, west and north of the player.
    CHECK(plan.points[0].x == doctest::Approx(kCenter.x + placement.distance));
    CHECK(plan.points[2].x == doctest::Approx(kCenter.x - placement.distance));

    // A quarter turn moves the first point to the south.
    const float quarter = 0.5f * std::numbers::pi_v<float>;
    const SummonPoints turned = horde::PlanSummon(kCenter, kFarBoss, kArena, placement, quarter);
    CHECK(turned.points[0].y == doctest::Approx(kCenter.y + placement.distance));
}

TEST_CASE("by a wall or in a corner, a point pushed inside the walls onto the player is dropped") {
    const SummonPlacement placement;
    // Against the west wall: the west point would land on the player once pushed inside, so it is dropped;
    // the other three ring the player on the open side, and the ones beside the player run along the wall.
    const Vec2 byWall{kArena.min.x + 60.0f, kCenter.y};
    const SummonPoints wall = horde::PlanSummon(byWall, kFarBoss, kArena, placement, 0.0f);
    CHECK(wall.count == 3);
    for (std::size_t i = 0; i < wall.count; ++i) {
        CHECK(InsideWalls(wall.points[i], placement.wallMargin));
        CHECK(horde::Length(wall.points[i] - byWall) >= placement.minPlayerDistance);
    }

    // In a corner, turned so two points fall outside the walls: pushed back in, they would land on the player.
    const Vec2 corner{kArena.min.x + 50.0f, kArena.min.y + 50.0f};
    const float eighth = 0.25f * std::numbers::pi_v<float>;
    const SummonPoints tight = horde::PlanSummon(corner, kFarBoss, kArena, placement, eighth);
    CHECK(tight.count < 4);
    CHECK(tight.count >= 1);
    for (std::size_t i = 0; i < tight.count; ++i) {
        CHECK(InsideWalls(tight.points[i], placement.wallMargin));
        CHECK(horde::Length(tight.points[i] - corner) >= placement.minPlayerDistance);
    }
}

TEST_CASE("a point that would put a hairball inside the boss is dropped") {
    const SummonPlacement placement;
    // The boss 250 units east of the player: the east point lands right on it.
    const SummonPoints plan = horde::PlanSummon(kCenter, kCenter + Vec2{250.0f, 0.0f}, kArena, placement, 0.0f);
    CHECK(plan.count == 3);
    for (std::size_t i = 0; i < plan.count; ++i) {
        CHECK(horde::Length(plan.points[i] - (kCenter + Vec2{250.0f, 0.0f})) >= placement.minBossDistance);
    }
}

TEST_CASE("a spawn marker waits its time, then lets go") {
    horde::SpawnMarker marker = horde::MakeSpawnMarker(kCenter, 0.75f);
    int steps = 0;
    while (horde::StepSpawnMarker(marker, kDt)) {
        ++steps;
    }
    CHECK(steps + 1 == doctest::Approx(45).epsilon(0.03));
}

TEST_CASE("a summon puts its marks down as the howl starts, then holds still, recovers and stalks") {
    World world = AboutToSummon(Vec2{500.0f, 0.0f});
    Step(world, 1);
    CHECK(world.spawnMarkers.Size() == 4);
    REQUIRE(world.boss.attack);
    CHECK(world.boss.attack->stage == AttackStage::Warning);
    // Only once.
    Step(world, 20);
    CHECK(world.spawnMarkers.Size() == 4);

    // 0.75 s of howl, 0.2 s of the active stage and 1 s of recovery.
    int steps = 21;
    while (world.boss.attack && steps < 200) {
        Step(world, 1);
        ++steps;
    }
    CHECK(steps == doctest::Approx(117).epsilon(0.03));
    CHECK(world.boss.motion.position == kCenter);
}

TEST_CASE("each mark lets out a hairball where it was, which shakes itself, then chases the player") {
    World world = AboutToSummon(Vec2{500.0f, 0.0f});
    Step(world, 1);
    REQUIRE(world.spawnMarkers.Size() == 4);
    std::array<Vec2, 4> marks{};
    for (std::size_t i = 0; i < 4; ++i) {
        marks[i] = world.spawnMarkers[i].position;
    }
    Step(world, 43);
    CHECK(world.enemies.Size() == 0);
    Step(world, 2);
    REQUIRE(world.enemies.Size() == 4);
    CHECK(world.spawnMarkers.Size() == 0);
    for (const Enemy& hairball : world.enemies) {
        bool onAMark = false;
        for (const Vec2 mark : marks) {
            onAMark = onAMark || horde::Length(hairball.motion.position - mark) < 1.0f;
        }
        CHECK(onAMark);
        CHECK(hairball.stunSeconds > 0.0f);
    }
    // After the shake they close in on the player.
    const float before = horde::Length(world.enemies[0].motion.position - world.player.motion.position);
    Step(world, 60);
    CHECK(horde::Length(world.enemies[0].motion.position - world.player.motion.position) < before);
}

TEST_CASE("a summon brings fewer hairballs when the arena already holds as many as the boss allows") {
    World world = AboutToSummon(Vec2{500.0f, 0.0f});
    // Seven hairballs held still in the far corner.
    for (int i = 0; i < 7; ++i) {
        Enemy hairball;
        hairball.motion = horde::MotionAt(Vec2{100.0f + 40.0f * static_cast<float>(i), 100.0f});
        hairball.stats.maxSpeed = 0.0f;
        world.enemies.Spawn(hairball);
    }
    Step(world, 60);
    CHECK(world.enemies.Size() == static_cast<std::size_t>(world.boss.summon.maxAlive));
}

TEST_CASE("in a frenzy a summon brings six hairballs") {
    World world = AboutToSummon(Vec2{500.0f, 0.0f});
    horde::EnterFrenzy(world.boss);
    Step(world, 1);
    CHECK(world.spawnMarkers.Size() == 6);
}

TEST_CASE("summoned hairballs spit, each on a beat of its own") {
    World world = AboutToSummon(Vec2{500.0f, 0.0f});
    Step(world, 46);
    REQUIRE(world.enemies.Size() == 4);
    const horde::SummonStats& stats = world.boss.summon;
    float earliest = 1.0e9f;
    float latest = -1.0e9f;
    for (const Enemy& hairball : world.enemies) {
        CHECK(hairball.spits);
        CHECK(hairball.secondsUntilSpit >= stats.firstSpitMinSeconds - 0.05f);
        CHECK(hairball.secondsUntilSpit <= stats.firstSpitMaxSeconds);
        earliest = std::min(earliest, hairball.secondsUntilSpit);
        latest = std::max(latest, hairball.secondsUntilSpit);
    }
    CHECK(latest - earliest > 0.1f);
}

TEST_CASE("a spitting hairball sends a strand at the player, then waits its interval for the next") {
    World world = horde::CreateWorld(kArena, kView);
    world.boss.health.current = 0; // Out of the way.
    world.player.motion = horde::MotionAt(kCenter + Vec2{0.0f, 200.0f});
    Enemy hairball;
    hairball.motion = horde::MotionAt(kCenter);
    hairball.stats.maxSpeed = 0.0f;
    hairball.spits = true;
    hairball.secondsUntilSpit = 0.5f;
    world.enemies.Spawn(hairball);

    int steps = 0;
    while (world.hostileProjectiles.Size() == 0 && steps < 120) {
        Step(world, 1);
        ++steps;
    }
    CHECK(steps == doctest::Approx(30).epsilon(0.05));
    REQUIRE(world.hostileProjectiles.Size() == 1);
    const horde::Projectile& strand = world.hostileProjectiles[0];
    const horde::SummonStats& stats = world.boss.summon;
    CHECK(strand.motion.velocity.y == doctest::Approx(stats.spitSpeed));
    CHECK(strand.motion.velocity.x == doctest::Approx(0.0f));
    CHECK(strand.damage == stats.spitDamage);
    CHECK(strand.source == horde::DamageSource::Hairball);

    // It hits the player, and the next one leaves three seconds after the first.
    int sinceFirst = 0;
    while (world.player.health.current == world.player.health.max && sinceFirst < 60) {
        Step(world, 1);
        ++sinceFirst;
    }
    CHECK(world.player.health.current == world.player.health.max - stats.spitDamage);
    CHECK(world.lastHitBy == horde::DamageSource::Hairball);
    REQUIRE(world.hostileProjectiles.Size() == 0);
    while (world.hostileProjectiles.Size() == 0 && sinceFirst < 400) {
        Step(world, 1);
        ++sinceFirst;
    }
    CHECK(sinceFirst + 1 == doctest::Approx(180).epsilon(0.02));
}

TEST_CASE("a hairball the boss did not summon never spits") {
    World world = horde::CreateWorld(kArena, kView);
    world.boss.health.current = 0;
    Enemy hairball;
    hairball.motion = horde::MotionAt(kCenter);
    world.enemies.Spawn(hairball);
    Step(world, 300);
    CHECK(world.hostileProjectiles.Size() == 0);
}
