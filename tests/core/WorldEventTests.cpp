#include <algorithm>
#include <cstddef>

#include <doctest/doctest.h>

#include "PredictableBoss.h"
#include "core/AttackChoice.h"
#include "core/World.h"

using horde::Enemy;
using horde::PlayerCommands;
using horde::Vec2;
using horde::World;
using horde::WorldEvent;
using horde::WorldEventKind;

namespace {

constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

World NewFight() {
    World world = horde::CreateWorld(horde::kBossArena, kView);
    world.boss = horde::test::Predictable(world.boss);
    return world;
}

void Step(World& world, const PlayerCommands& commands, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepWorld(world, commands, kDt);
    }
}

std::size_t Count(const World& world, WorldEventKind kind) {
    return static_cast<std::size_t>(std::count_if(world.events.begin(), world.events.end(),
                                                  [kind](const WorldEvent& event) { return event.kind == kind; }));
}

const WorldEvent* Find(const World& world, WorldEventKind kind) {
    const auto found = std::find_if(world.events.begin(), world.events.end(),
                                    [kind](const WorldEvent& event) { return event.kind == kind; });
    return found == world.events.end() ? nullptr : found;
}

// Steps until an event of `kind` shows up, for at most `maxSteps`.
const WorldEvent* StepUntil(World& world, WorldEventKind kind, int maxSteps) {
    for (int i = 0; i < maxSteps && !Find(world, kind); ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    return Find(world, kind);
}

// Fires one pistol bullet at the boss, which the player starts out facing, and waits for it to land.
void ShootBossOnce(World& world) {
    const int hitsBefore = world.shotsHit;
    Step(world, PlayerCommands{.firePressed = true}, 1);
    for (int i = 0; i < 40 && world.shotsHit == hitsBefore; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    REQUIRE(world.shotsHit == hitsBefore + 1);
}

} // namespace

TEST_CASE("a new world has nothing to tell, and a quiet step adds nothing") {
    World world = NewFight();
    CHECK(world.events.Empty());
    Step(world, PlayerCommands{}, 10);
    CHECK(world.events.Empty());
}

TEST_CASE("a shot records where it left the gun, which way it went and how hard it hits") {
    World world = NewFight();
    Step(world, PlayerCommands{.firePressed = true}, 1);
    const WorldEvent* shot = Find(world, WorldEventKind::ShotFired);
    REQUIRE(shot);
    CHECK(shot->position == world.bullets[0].motion.previousPosition);
    CHECK(shot->direction == world.player.facing);
    CHECK(shot->amount == horde::kPistol.damage);
}

TEST_CASE("a hit on the player records the damage, and the hit that empties the bar records the death") {
    World world = NewFight();
    world.boss.health.current = 0; // Out of the way: only the hairball touches the player.
    const Vec2 player = world.player.motion.position;
    world.enemies.Spawn(Enemy{.motion = horde::MotionAt(player + Vec2{25.0f, 0.0f})});
    Step(world, PlayerCommands{}, 1);
    const WorldEvent* hurt = Find(world, WorldEventKind::PlayerHurt);
    REQUIRE(hurt);
    CHECK(hurt->amount == 10);
    CHECK(Count(world, WorldEventKind::PlayerKilled) == 0);

    world.events.Clear();
    world.player.health.current = 5;
    world.player.health.invulnerableSeconds = 0.0f;
    Step(world, PlayerCommands{}, 1);
    CHECK(horde::IsDead(world.player.health));
    CHECK(Count(world, WorldEventKind::PlayerKilled) == 1);
    CHECK(Count(world, WorldEventKind::PlayerHurt) == 0);
}

TEST_CASE("starting a dash records where it left from and which way it went, once per dash") {
    World world = NewFight();
    const Vec2 start = world.player.motion.position;
    Step(world, PlayerCommands{.move = Vec2{1.0f, 0.0f}, .dashPressed = true}, 1);
    const WorldEvent* dash = Find(world, WorldEventKind::DashStarted);
    REQUIRE(dash);
    CHECK(dash->position == start);
    CHECK(dash->direction == Vec2{1.0f, 0.0f});

    world.events.Clear();
    Step(world, PlayerCommands{.move = Vec2{1.0f, 0.0f}, .dashPressed = true}, 1);
    CHECK(Count(world, WorldEventKind::DashStarted) == 0);
}

TEST_CASE("a dash through a strand records nothing on the player, because nothing landed") {
    World world = NewFight();
    world.boss.health.current = 0;
    horde::Projectile strand = horde::LaunchProjectile(world.player.motion.position + Vec2{-60.0f, 0.0f},
                                                       Vec2{1.0f, 0.0f}, 300.0f, 400.0f);
    strand.damage = 15;
    world.hostileProjectiles.Spawn(strand);
    Step(world, PlayerCommands{.move = Vec2{0.0f, 1.0f}, .dashPressed = true}, 1);
    Step(world, PlayerCommands{}, 6);
    CHECK(Count(world, WorldEventKind::PlayerHurt) == 0);
}

TEST_CASE("bullets on the shield record a strike, the one that breaks it a break, and the next ones a hurt boss") {
    World world = NewFight();
    // An SMG bullet bounces off the Shell and records 0 damage.
    Step(world, PlayerCommands{.selectSlot = 1}, 1);
    ShootBossOnce(world);
    const WorldEvent* bounced = Find(world, WorldEventKind::ShieldStruck);
    REQUIRE(bounced);
    CHECK(bounced->amount == 0);

    // A Barrier soaks up a pistol bullet without breaking, recording the bullet's damage.
    world.events.Clear();
    Step(world, PlayerCommands{.selectSlot = 0}, 1);
    world.boss.shield.kind = horde::ShieldKind::Barrier;
    world.boss.shield.strength = world.boss.shield.stats.barrierStrength;
    ShootBossOnce(world);
    const WorldEvent* absorbed = Find(world, WorldEventKind::ShieldStruck);
    REQUIRE(absorbed);
    CHECK(absorbed->amount == horde::kPistol.damage);
    CHECK(Count(world, WorldEventKind::ShieldBroken) == 0);

    // A Shell one bullet from breaking.
    world.events.Clear();
    world.boss.shield.kind = horde::ShieldKind::Shell;
    world.boss.shield.strength = static_cast<float>(horde::kPistol.damage);
    ShootBossOnce(world);
    const WorldEvent* broken = Find(world, WorldEventKind::ShieldBroken);
    REQUIRE(broken);
    CHECK(broken->direction.y == doctest::Approx(-1.0f));

    world.events.Clear();
    ShootBossOnce(world);
    const WorldEvent* hurt = Find(world, WorldEventKind::BossHurt);
    REQUIRE(hurt);
    CHECK(hurt->amount == horde::kPistol.damage);
    CHECK(Count(world, WorldEventKind::ShieldStruck) == 0);
}

TEST_CASE("the bullet that kills a hairball records where it died") {
    World world = NewFight();
    world.boss.health.current = 0;
    const Vec2 spot = world.player.motion.position + Vec2{0.0f, -150.0f};
    Enemy* hairball = world.enemies.Spawn(Enemy{.motion = horde::MotionAt(spot)});
    hairball->health.current = 1;
    Step(world, PlayerCommands{.aimAt = spot, .firePressed = true}, 1);
    const WorldEvent* killed = StepUntil(world, WorldEventKind::HairballKilled, 30);
    REQUIRE(killed);
    CHECK(horde::Length(killed->position - spot) < 30.0f);
    CHECK(world.enemies.Empty());
}

TEST_CASE("the end of each phase is recorded: the transformation, its roar, and the victory") {
    World world = NewFight();
    horde::BreakShield(world.boss.shield);
    world.boss.health.current = horde::kPistol.damage;
    ShootBossOnce(world);
    CHECK(Count(world, WorldEventKind::FirstPhaseEnded) == 1);
    CHECK(Count(world, WorldEventKind::BossDefeated) == 0);

    world.events.Clear();
    CHECK(StepUntil(world, WorldEventKind::BossRoared, 200));

    world.events.Clear();
    horde::BreakShield(world.boss.shield);
    world.boss.health.current = horde::kPistol.damage;
    world.player.motion = horde::MotionAt(world.boss.motion.position + Vec2{0.0f, 300.0f});
    world.player.facing = Vec2{0.0f, -1.0f};
    world.boss.attack.reset();
    world.boss.secondsUntilAttack = 5.0f;
    ShootBossOnce(world);
    CHECK(horde::IsBossDefeated(world));
    CHECK(Count(world, WorldEventKind::BossDefeated) == 1);
}

TEST_CASE("the Debug shortcuts record the events the real thing would") {
    World world = NewFight();
    horde::BreakBossShield(world);
    CHECK(Count(world, WorldEventKind::ShieldBroken) == 1);
    horde::TransformBoss(world);
    CHECK(Count(world, WorldEventKind::FirstPhaseEnded) == 1);
    // Only the first phase transforms: a second press does nothing, and records nothing.
    horde::TransformBoss(world);
    CHECK(Count(world, WorldEventKind::FirstPhaseEnded) == 1);
    horde::DefeatBoss(world);
    CHECK(Count(world, WorldEventKind::BossDefeated) == 1);
}

TEST_CASE("a charge into the wall records the crash, along the run") {
    World world = NewFight();
    // The player due east, so the charge runs east and meets the wall 760 units away.
    world.player.motion = horde::MotionAt(world.boss.motion.position + Vec2{300.0f, 0.0f});
    world.boss.attack = horde::BossAttack{};
    const WorldEvent* crash = StepUntil(world, WorldEventKind::BossHitWall, 180);
    REQUIRE(crash);
    CHECK(crash->direction.x == doctest::Approx(1.0f));
    CHECK(crash->position.x == doctest::Approx(world.arena.max.x - world.boss.radius));
}

TEST_CASE("a leap records its landing where the boss came down") {
    World world = NewFight();
    world.boss.nextAttack = horde::AttackKind::Leap;
    world.boss.secondsUntilAttack = 0.0f;
    const WorldEvent* landing = StepUntil(world, WorldEventKind::BossLanded, 180);
    REQUIRE(landing);
    // Where the shadow locked. The boss came down on the standing player, so the push apart after the
    // landing has already moved its body a little off that point.
    REQUIRE(world.boss.attack);
    CHECK(landing->position == world.boss.attack->target);
}

TEST_CASE("events pile up until the game clears them, and a full list drops new ones without touching the fight") {
    World world = NewFight();
    world.boss.health.current = 0;
    const Vec2 away = world.player.motion.position + Vec2{0.0f, 300.0f};
    Step(world, PlayerCommands{.selectSlot = 1}, 1);
    // Ten seconds of the SMG into the south wall: 100 shots, more than the list holds.
    Step(world, PlayerCommands{.aimAt = away, .fireHeld = true}, 600);
    CHECK(world.shotsFired == 100);
    CHECK(world.events.Size() == horde::kMaxWorldEvents);

    world.events.Clear();
    Step(world, PlayerCommands{.aimAt = away, .fireHeld = true}, 6);
    CHECK_FALSE(world.events.Empty());
}
