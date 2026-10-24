#include <cmath>
#include <limits>

#include <doctest/doctest.h>

#include "core/Collision.h"
#include "core/World.h"

using horde::AttackStage;
using horde::Boss;
using horde::Enemy;
using horde::PlayerCommands;
using horde::Vec2;
using horde::World;

namespace {

constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

World NewFight() {
    return horde::CreateWorld(horde::kBossArena, kView);
}

void Step(World& world, const PlayerCommands& commands, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepWorld(world, commands, kDt);
    }
}

float Distance(Vec2 a, Vec2 b) {
    return horde::Length(b - a);
}

// The player just touching the boss from below.
void PlayerTouchingBoss(World& world) {
    const float contact = world.boss.radius + world.player.radius;
    world.player.motion = horde::MotionAt(world.boss.motion.position + Vec2{0.0f, contact});
}

// A boss that never attacks, for the tests about its walk and its body.
void OnlyWalks(Boss& boss) {
    boss.secondsUntilAttack = std::numeric_limits<float>::infinity();
}

// The boss in the middle, starting its charge warning on the next step, with the player standing still
// `distance` units south of it, right in its path.
World AboutToCharge(float distance) {
    World world = NewFight();
    world.player.motion = horde::MotionAt(world.boss.motion.position + Vec2{0.0f, distance});
    world.boss.secondsUntilAttack = 0.0f;
    return world;
}

// Steps until the boss's attack reaches `stage`, for at most two seconds.
void StepUntilStage(World& world, const PlayerCommands& commands, AttackStage stage) {
    for (int i = 0; i < 120 && !(world.boss.attack && world.boss.attack->stage == stage); ++i) {
        horde::StepWorld(world, commands, kDt);
    }
    REQUIRE(world.boss.attack);
    REQUIRE(world.boss.attack->stage == stage);
}

// Whether `point` is on the screen, `inset` units or more from every edge of the view.
bool InView(const World& world, Vec2 point, float inset) {
    const Vec2 half = world.camera.viewSize * 0.5f;
    const Vec2 offset = point - world.camera.center;
    return std::abs(offset.x) <= half.x - inset && std::abs(offset.y) <= half.y - inset;
}

} // namespace

TEST_CASE("the boss walks toward the player, slower than a hairball") {
    World world = NewFight();
    const float before = Distance(world.boss.motion.position, world.player.motion.position);

    Step(world, PlayerCommands{}, 60);
    CHECK(Distance(world.boss.motion.position, world.player.motion.position) < before);
    CHECK(horde::Length(world.boss.motion.velocity) <= world.boss.stats.maxSpeed + 0.01f);
    CHECK(world.boss.stats.maxSpeed < Enemy{}.stats.maxSpeed);
}

TEST_CASE("the boss stops when it touches the player, and pushes the player rather than being pushed") {
    World world = NewFight();
    OnlyWalks(world.boss);
    // The player stands still until the boss arrives, then runs straight into it for a second.
    Step(world, PlayerCommands{}, 360);
    const float contact = world.boss.radius + world.player.radius;
    REQUIRE(Distance(world.boss.motion.position, world.player.motion.position) <= contact + horde::kContactSlack);
    CHECK(horde::Length(world.boss.motion.velocity) < 0.01f);

    const Vec2 bossBefore = world.boss.motion.position;
    world.player.health.invulnerableSeconds = 100.0f; // Only the shoving is under test here.
    Step(world, PlayerCommands{.move = {0.0f, -1.0f}}, 60);
    CHECK(Distance(world.boss.motion.position, bossBefore) < 15.0f);
    CHECK(Distance(world.boss.motion.position, world.player.motion.position) >= contact - 0.01f);
}

TEST_CASE("touching the boss hurts the player by the boss's contact damage") {
    World world = NewFight();
    PlayerTouchingBoss(world);
    Step(world, PlayerCommands{}, 1);
    CHECK(world.player.health.current == world.player.health.max - world.boss.contactDamage);
}

TEST_CASE("when the boss and a hairball touch the player in the same step, the boss's hit lands") {
    World world = NewFight();
    PlayerTouchingBoss(world);
    const float hairballContact = world.player.radius + Enemy{}.radius;
    world.enemies.Spawn(Enemy{.motion = horde::MotionAt(world.player.motion.position + Vec2{hairballContact, 0.0f})});

    Step(world, PlayerCommands{}, 1);
    CHECK(world.player.health.current == world.player.health.max - world.boss.contactDamage);
}

TEST_CASE("a bullet takes health from the boss and flashes it, but does not push it") {
    World world = NewFight();
    const Boss& boss = world.boss;
    // The player starts aiming at the boss, 400 units north.
    Step(world, PlayerCommands{.firePressed = true}, 1);
    for (int i = 0; i < 30 && boss.health.current == boss.health.max; ++i) {
        Step(world, PlayerCommands{}, 1);
    }

    REQUIRE(boss.health.current == boss.health.max - horde::kPistol.damage);
    CHECK(boss.hitFlashSeconds > 0.0f);
    // The bullet flew north. The boss keeps walking south, at the player, as if nothing happened.
    CHECK(boss.motion.velocity.y > 0.0f);
    CHECK(boss.motion.velocity.x == doctest::Approx(0.0f));
}

TEST_CASE("a hairball in front of the boss takes the bullet meant for the boss") {
    World world = NewFight();
    const Vec2 between = horde::Lerp(world.player.motion.position, world.boss.motion.position, 0.5f);
    Enemy& hairball = *world.enemies.Spawn(Enemy{.motion = horde::MotionAt(between), .stunSeconds = 1.0f});

    Step(world, PlayerCommands{.firePressed = true}, 1);
    Step(world, PlayerCommands{}, 30);
    CHECK(hairball.health.current == hairball.health.max - horde::kPistol.damage);
    CHECK(world.boss.health.current == world.boss.health.max);
}

TEST_CASE("a dash goes through the boss without a scratch") {
    World world = NewFight();
    PlayerTouchingBoss(world);

    Step(world, PlayerCommands{.move = {0.0f, -1.0f}, .dashPressed = true}, 9);
    const Boss& boss = world.boss;
    CHECK(world.player.motion.position.y < boss.motion.position.y - (boss.radius + world.player.radius));
    CHECK(world.player.health.current == world.player.health.max);
}

TEST_CASE("a dead boss does not chase, block, take bullets or hurt") {
    World world = NewFight();
    Boss& boss = world.boss;
    boss.health.current = 0;
    const Vec2 corpse = boss.motion.position;

    // The player walks straight through the body, shooting at it, and comes out unhurt on the far side.
    Step(world, PlayerCommands{.move = {0.0f, -1.0f}, .fireHeld = true, .firePressed = true}, 150);
    CHECK(boss.motion.position == corpse);
    CHECK(world.player.motion.position.y < corpse.y - boss.radius);
    CHECK(world.player.health.current == world.player.health.max);
    CHECK(boss.health.current == 0);
}

TEST_CASE("a new fight starts with the camera settled between the player and the boss") {
    const World world = NewFight();
    const Vec2 player = world.player.motion.position;
    const Vec2 boss = world.boss.motion.position;
    CHECK(world.camera.previousCenter == world.camera.center);
    CHECK(world.camera.center.y < player.y);
    CHECK(world.camera.center.y > boss.y);
    CHECK(Distance(world.camera.center, player) < Distance(world.camera.center, boss));
    CHECK(InView(world, player, 100.0f));
    CHECK(InView(world, boss, 0.0f));
}

TEST_CASE("the camera keeps a far boss in mind without pushing the player off screen") {
    World world = NewFight();
    // The boss held in one corner, the player standing in the opposite one. Without the cap on how far
    // the camera leans toward the boss, the player would end up at the edge of the screen.
    world.boss.motion = horde::MotionAt(world.arena.min + Vec2{100.0f, 100.0f});
    world.boss.stats.maxSpeed = 0.0f;
    OnlyWalks(world.boss);

    world.player.motion = horde::MotionAt(world.arena.max - Vec2{100.0f, 100.0f});
    Step(world, PlayerCommands{}, 120);
    CHECK(InView(world, world.player.motion.position, 150.0f));
}

TEST_CASE("once the boss is dead, the camera goes back to the player alone") {
    World world = NewFight();
    world.boss.health.current = 0;
    // Where the view can center on the player without meeting a wall.
    world.player.motion = horde::MotionAt(horde::Center(world.arena) + Vec2{0.0f, 200.0f});
    Step(world, PlayerCommands{}, 120);
    CHECK(world.camera.center.x == doctest::Approx(world.player.motion.position.x).epsilon(0.01));
    CHECK(world.camera.center.y == doctest::Approx(world.player.motion.position.y).epsilon(0.01));
}

TEST_CASE("a charge that catches the player takes the charge's damage and throws them out of its path") {
    World world = AboutToCharge(200.0f);
    const horde::Player& player = world.player;
    StepUntilStage(world, PlayerCommands{}, AttackStage::Active);
    for (int i = 0; i < 30 && player.health.current == player.health.max; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    CHECK(player.health.current == player.health.max - world.boss.charge.damage);
    // The charge runs south, so out of its path is sideways.
    CHECK(std::abs(player.motion.velocity.x) >= world.boss.charge.knockback * 0.9f);
}

TEST_CASE("the charge runs through the player instead of carrying them along") {
    World world = AboutToCharge(200.0f);
    const float startY = world.player.motion.position.y;
    StepUntilStage(world, PlayerCommands{}, AttackStage::Recovery);

    // The boss ran from the middle to the south wall, 460 units, right through where the player stood.
    const Boss& boss = world.boss;
    CHECK(boss.attack->stunned);
    CHECK(boss.motion.position.y == doctest::Approx(world.arena.max.y - boss.radius));
    CHECK(std::abs(world.player.motion.position.y - startY) < 30.0f);
}

TEST_CASE("a dash through the charge takes no damage") {
    World world = AboutToCharge(200.0f);
    StepUntilStage(world, PlayerCommands{}, AttackStage::Active);
    Step(world, PlayerCommands{.move = {0.0f, -1.0f}, .dashPressed = true}, 1);
    Step(world, PlayerCommands{}, 60);
    CHECK(world.player.health.current == world.player.health.max);
    CHECK(world.player.motion.position.y < world.boss.motion.position.y);
}

TEST_CASE("the boss counts as charging only while it runs") {
    World world = AboutToCharge(200.0f);
    StepUntilStage(world, PlayerCommands{}, AttackStage::Warning);
    CHECK_FALSE(horde::IsCharging(world.boss));
    StepUntilStage(world, PlayerCommands{}, AttackStage::Active);
    CHECK(horde::IsCharging(world.boss));
    StepUntilStage(world, PlayerCommands{}, AttackStage::Recovery);
    CHECK_FALSE(horde::IsCharging(world.boss));
}
