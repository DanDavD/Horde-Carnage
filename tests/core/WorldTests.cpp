#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include <doctest/doctest.h>

#include "core/Collision.h"
#include "core/World.h"

using horde::Arena;
using horde::Enemy;
using horde::PlayerCommands;
using horde::Vec2;
using horde::World;

namespace {

// The same arena and view as the game, so these tests describe what the player actually sees.
constexpr Arena kArena = horde::kBossArena;
constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

void Step(World& world, const PlayerCommands& commands, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepWorld(world, commands, kDt);
    }
}

// Most tests only move. Nothing aims, so the player faces where it walks.
void Step(World& world, Vec2 move, int steps) {
    Step(world, PlayerCommands{.move = move}, steps);
}

// The player alone in the arena. A dead boss takes no part in the simulation (it does not chase, block,
// take bullets or hurt), so the tests about the player and the hairballs start from a world where it is
// dead, and nothing but what the test adds can interfere.
World PlayerAlone() {
    World world = horde::CreateWorld(kArena, kView);
    world.boss.health.current = 0;
    return world;
}

// Six hairballs evenly spaced on a ring of 520 units around the player, the crowd the crowd tests need.
World SurroundedByHairballs() {
    World world = PlayerAlone();
    const Vec2 center = world.player.motion.position;
    for (int i = 0; i < 6; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / 6.0f;
        const Vec2 offset{std::cos(angle) * 520.0f, std::sin(angle) * 520.0f};
        world.enemies.Spawn(Enemy{.motion = horde::MotionAt(center + offset)});
    }
    return world;
}

// A single hairball at `position`, for tests about one enemy.
Enemy& OnlyEnemyAt(World& world, Vec2 position) {
    world.enemies.Clear();
    return *world.enemies.Spawn(Enemy{.motion = horde::MotionAt(position)});
}

float DistanceToPlayer(const World& world, const Enemy& enemy) {
    return horde::Length(enemy.motion.position - world.player.motion.position);
}

// How far the two most overlapping bodies sink into each other. 0 when nobody overlaps.
float DeepestOverlap(const World& world) {
    float deepest = 0.0f;
    const auto check = [&deepest](Vec2 a, float radiusA, Vec2 b, float radiusB) {
        deepest = std::max(deepest, radiusA + radiusB - horde::Length(b - a));
    };
    const horde::Player& player = world.player;
    for (std::size_t i = 0; i < world.enemies.Size(); ++i) {
        const Enemy& enemy = world.enemies[i];
        check(player.motion.position, player.radius, enemy.motion.position, enemy.radius);
        for (std::size_t j = i + 1; j < world.enemies.Size(); ++j) {
            const Enemy& other = world.enemies[j];
            check(enemy.motion.position, enemy.radius, other.motion.position, other.radius);
        }
    }
    return deepest;
}

bool InsideArena(Vec2 position, float radius) {
    return position.x >= kArena.min.x + radius && position.x <= kArena.max.x - radius &&
           position.y >= kArena.min.y + radius && position.y <= kArena.max.y - radius;
}

} // namespace

TEST_CASE("a new world puts the boss in the middle and the player south of it, facing it") {
    const World world = horde::CreateWorld(kArena, kView);
    CHECK(world.boss.motion.position == horde::Center(kArena));
    CHECK(world.player.motion.position == horde::Center(kArena) + Vec2{0.0f, horde::kStartingDistance});
    CHECK(world.player.motion.previousPosition == world.player.motion.position);
    CHECK(world.boss.motion.previousPosition == world.boss.motion.position);
    CHECK(world.player.facing == Vec2{0.0f, -1.0f});

    CHECK(world.boss.health.current == world.boss.health.max);
    CHECK(world.enemies.Empty());
    CHECK(world.bullets.Empty());
    CHECK(InsideArena(world.player.motion.position, world.player.radius));
    CHECK(InsideArena(world.boss.motion.position, world.boss.radius));
}

TEST_CASE("a step moves and turns the player by the movement rules") {
    World world = horde::CreateWorld(kArena, kView);
    horde::Motion expected = world.player.motion;
    horde::StepMovement(expected, Vec2{1.0f, 0.0f}, world.player.stats, kDt);

    Step(world, Vec2{1.0f, 0.0f}, 1);
    CHECK(world.player.motion.position == expected.position);
    CHECK(world.player.motion.velocity == expected.velocity);
    CHECK(world.player.facing == Vec2{1.0f, 0.0f});
}

TEST_CASE("a step moves the camera after the player") {
    World world = horde::CreateWorld(kArena, kView);
    const Vec2 before = world.camera.center;
    Step(world, Vec2{1.0f, 0.0f}, 1);
    CHECK(world.camera.previousCenter == before);
    CHECK(world.camera.center.x > before.x);
}

TEST_CASE("hairballs walk toward the player") {
    World world = SurroundedByHairballs();
    std::vector<float> before;
    for (const Enemy& enemy : world.enemies) {
        before.push_back(DistanceToPlayer(world, enemy));
    }

    Step(world, Vec2{}, 60);
    for (std::size_t i = 0; i < world.enemies.Size(); ++i) {
        CHECK(DistanceToPlayer(world, world.enemies[i]) < before[i]);
    }
}

TEST_CASE("hairballs come to rest touching the player, without sinking into it") {
    World world = SurroundedByHairballs();
    Step(world, Vec2{}, 600);

    for (const Enemy& enemy : world.enemies) {
        const float contact = enemy.radius + world.player.radius;
        CHECK(DistanceToPlayer(world, enemy) >= contact - 0.01f);
        CHECK(DistanceToPlayer(world, enemy) <= contact + horde::kContactSlack);
        CHECK(horde::Length(enemy.motion.velocity) < 0.01f);
    }
    CHECK(DeepestOverlap(world) < 0.01f);
}

TEST_CASE("hairballs that follow a running player spread out instead of piling up") {
    World world = SurroundedByHairballs();
    Step(world, Vec2{}, 300);
    Step(world, Vec2{1.0f, 0.0f}, 120);
    Step(world, Vec2{}, 240);
    CHECK(DeepestOverlap(world) < 0.5f);
}

TEST_CASE("a crowd leans on a standing player, but far too slowly to notice") {
    // The hairballs in the second row cannot touch the player, so they keep walking into the first row,
    // and a little of that push reaches the player. It must stay under 2% of the player's top speed.
    World world = SurroundedByHairballs();
    Step(world, Vec2{}, 300);
    Step(world, Vec2{1.0f, 0.0f}, 120);

    const Vec2 before = world.player.motion.position;
    Step(world, Vec2{}, 600);
    const float driftPerSecond = horde::Length(world.player.motion.position - before) / 10.0f;
    CHECK(driftPerSecond < 0.02f * world.player.stats.maxSpeed);
}

TEST_CASE("the player can shove through a hairball, but slower than running free") {
    World free = PlayerAlone();
    World blocked = PlayerAlone();
    OnlyEnemyAt(blocked, blocked.player.motion.position + Vec2{60.0f, 0.0f});

    const float start = free.player.motion.position.x;
    Step(free, Vec2{1.0f, 0.0f}, 60);
    Step(blocked, Vec2{1.0f, 0.0f}, 60);

    const float freeDistance = free.player.motion.position.x - start;
    const float blockedDistance = blocked.player.motion.position.x - start;
    CHECK(blockedDistance < freeDistance);
    CHECK(blockedDistance > 0.5f * freeDistance);
    CHECK(blocked.enemies[0].motion.position.x > blocked.player.motion.position.x);
}

TEST_CASE("nobody leaves the arena when the player runs into a corner") {
    World world = SurroundedByHairballs();
    world.boss.health.current = world.boss.health.max;
    Step(world, Vec2{1.0f, 1.0f}, 600);

    CHECK(InsideArena(world.player.motion.position, world.player.radius));
    CHECK(InsideArena(world.boss.motion.position, world.boss.radius));
    for (const Enemy& enemy : world.enemies) {
        CHECK(InsideArena(enemy.motion.position, enemy.radius));
    }
}

TEST_CASE("the same inputs always give the same world") {
    World a = SurroundedByHairballs();
    a.boss.health.current = a.boss.health.max;
    World b = a;
    const Vec2 moves[] = {{1.0f, 0.0f}, {0.0f, -1.0f}, {-0.5f, 0.5f}, {}};
    for (int i = 0; i < 400; ++i) {
        const PlayerCommands commands{.move = moves[(i / 50) % 4],
                                      .aimAt = Vec2{100.0f * static_cast<float>(i % 7), 900.0f},
                                      .fireHeld = i % 3 == 0,
                                      .firePressed = i % 10 == 0,
                                      .cycleWeapon = i % 120 == 0,
                                      .dashPressed = i % 90 == 0};
        horde::StepWorld(a, commands, kDt);
        horde::StepWorld(b, commands, kDt);
    }

    CHECK(a.player.motion.position == b.player.motion.position);
    CHECK(a.player.facing == b.player.facing);
    CHECK(a.player.health.current == b.player.health.current);
    CHECK(a.boss.motion.position == b.boss.motion.position);
    CHECK(a.boss.health.current == b.boss.health.current);
    CHECK(a.shotsFired == b.shotsFired);
    REQUIRE(a.bullets.Size() == b.bullets.Size());
    REQUIRE(a.enemies.Size() == b.enemies.Size());
    for (std::size_t i = 0; i < a.enemies.Size(); ++i) {
        CHECK(a.enemies[i].motion.position == b.enemies[i].motion.position);
    }
}

TEST_CASE("the fight's clock counts simulated seconds") {
    World world = horde::CreateWorld(kArena, kView);
    Step(world, Vec2{}, 120);
    CHECK(world.elapsedSeconds == doctest::Approx(2.0f));
}

TEST_CASE("nothing hurts the player in the first second of the fight") {
    World world = horde::CreateWorld(kArena, kView);
    Step(world, Vec2{}, 60);
    CHECK(world.player.health.current == world.player.health.max);
}

TEST_CASE("a touching hairball hurts the player once per invulnerability window") {
    World world = PlayerAlone();
    horde::Player& player = world.player;
    OnlyEnemyAt(world, player.motion.position + Vec2{player.radius + 18.0f, 0.0f});
    const int damage = world.enemies[0].contactDamage;

    Step(world, Vec2{}, 1);
    CHECK(player.health.current == player.health.max - damage);

    // 0.7 s later the window (0.8 s) is still open.
    Step(world, Vec2{}, 42);
    CHECK(player.health.current == player.health.max - damage);

    // 0.9 s after the first hit it has closed, and the hairball is still touching.
    Step(world, Vec2{}, 12);
    CHECK(player.health.current == player.health.max - 2 * damage);
}

TEST_CASE("a whole crowd arriving at once still lands only one hit") {
    World world = SurroundedByHairballs();
    const int damage = world.enemies[0].contactDamage;
    int steps = 0;
    while (world.player.health.current == world.player.health.max && steps < 600) {
        Step(world, Vec2{}, 1);
        ++steps;
    }
    // The six hairballs arrive together, but the first touch opens the window and the other five miss.
    CHECK(world.player.health.current == world.player.health.max - damage);
}

TEST_CASE("the player faces the aim point, not the way it walks") {
    World world = horde::CreateWorld(kArena, kView);
    const Vec2 below = world.player.motion.position + Vec2{0.0f, 300.0f};
    Step(world, PlayerCommands{.move = {1.0f, 0.0f}, .aimAt = below}, 30);

    CHECK(world.player.motion.velocity.x > 0.0f);
    CHECK(world.player.facing.y == doctest::Approx(1.0f).epsilon(0.05));
}

TEST_CASE("the fire button fires the gun in hand") {
    World world = horde::CreateWorld(kArena, kView);
    Step(world, PlayerCommands{.firePressed = true}, 1);
    CHECK(world.shotsFired == 1);
    CHECK(world.bullets.Size() == 1);
    CHECK(horde::Equipped(world.player.loadout).muzzleFlashSeconds > 0.0f);
}

TEST_CASE("a bullet that hits counts toward accuracy, and a miss does not") {
    World world = horde::CreateWorld(kArena, kView);
    CHECK(horde::Accuracy(world) == 0.0f);

    // The player starts aiming at the boss, so the first shot hits.
    Step(world, PlayerCommands{.firePressed = true}, 1);
    Step(world, PlayerCommands{}, 30);
    REQUIRE(world.shotsHit == 1);

    // The second goes the other way, into the south wall.
    const Vec2 away = world.player.motion.position + Vec2{0.0f, 300.0f};
    Step(world, PlayerCommands{.aimAt = away, .firePressed = true}, 1);
    Step(world, PlayerCommands{.aimAt = away}, 60);
    CHECK(world.bullets.Empty());
    CHECK(world.shotsFired == 2);
    CHECK(world.shotsHit == 1);
    CHECK(horde::Accuracy(world) == doctest::Approx(0.5f));
}

TEST_CASE("holding the button fires the SMG steadily but the pistol only once") {
    World pistol = horde::CreateWorld(kArena, kView);
    Step(pistol, PlayerCommands{.fireHeld = true, .firePressed = true}, 1);
    Step(pistol, PlayerCommands{.fireHeld = true}, 59);
    CHECK(pistol.shotsFired == 1);

    World smg = horde::CreateWorld(kArena, kView);
    Step(smg, PlayerCommands{.selectSlot = 1}, 1);
    Step(smg, PlayerCommands{.fireHeld = true}, 60);
    CHECK(smg.shotsFired == 10);
}

TEST_CASE("weapon commands switch the gun in hand") {
    World world = horde::CreateWorld(kArena, kView);
    CHECK(world.player.loadout.equipped == 0);

    Step(world, PlayerCommands{.cycleWeapon = true}, 1);
    CHECK(world.player.loadout.equipped == 1);

    Step(world, PlayerCommands{.selectSlot = 0}, 1);
    CHECK(world.player.loadout.equipped == 0);
}

namespace {

// Aims along +x from the player, where the single-enemy tests put their hairballs.
PlayerCommands AimRight(const World& world, bool firePressed) {
    return PlayerCommands{.aimAt = world.player.motion.position + Vec2{1000.0f, 0.0f}, .firePressed = firePressed};
}

} // namespace

TEST_CASE("a shot leaves the muzzle and flies where the player aims") {
    World world = PlayerAlone();
    const horde::Player& player = world.player;
    const Vec2 muzzle = horde::MuzzlePoint(player.motion.position, Vec2{1.0f, 0.0f}, player.radius, horde::kPistol);

    Step(world, AimRight(world, true), 1);
    REQUIRE(world.bullets.Size() == 1);
    const horde::Projectile& bullet = world.bullets[0];
    CHECK(bullet.motion.previousPosition.x == doctest::Approx(muzzle.x));
    CHECK(bullet.motion.previousPosition.y == doctest::Approx(muzzle.y));
    CHECK(bullet.motion.velocity.x == doctest::Approx(horde::kPistol.bulletSpeed));
    CHECK(bullet.motion.velocity.y == doctest::Approx(0.0f));
}

TEST_CASE("a bullet that hits nothing is gone once it has flown its range") {
    World world = PlayerAlone();
    world.player.motion = horde::MotionAt(Vec2{kArena.min.x + 100.0f, horde::Center(kArena).y});
    Step(world, AimRight(world, true), 1);
    REQUIRE(world.bullets.Size() == 1);

    const int lifetimeSteps = static_cast<int>(horde::kPistol.range / horde::kPistol.bulletSpeed / kDt);
    Step(world, AimRight(world, false), lifetimeSteps - 1);
    CHECK(world.bullets.Size() == 1);
    Step(world, AimRight(world, false), 2);
    CHECK(world.bullets.Empty());
}

TEST_CASE("a bullet that reaches a wall is gone at once") {
    World world = PlayerAlone();
    world.player.motion = horde::MotionAt(Vec2{kArena.max.x - 60.0f, horde::Center(kArena).y});
    Step(world, AimRight(world, true), 1);
    Step(world, AimRight(world, false), 5);
    CHECK(world.bullets.Empty());
}

TEST_CASE("three pistol bullets kill a hairball, and it is removed") {
    World world = PlayerAlone();
    OnlyEnemyAt(world, world.player.motion.position + Vec2{250.0f, 0.0f});

    // Three clicks, each when the pistol is ready again, then time for the last bullet to arrive.
    for (int shot = 0; shot < 3; ++shot) {
        Step(world, AimRight(world, true), 1);
        Step(world, AimRight(world, false), 9);
    }
    Step(world, AimRight(world, false), 30);

    CHECK(world.enemies.Empty());
    CHECK(world.shotsFired == 3);
}

TEST_CASE("a hit flashes the hairball, stuns it and knocks it back along the bullet") {
    World world = PlayerAlone();
    Enemy& hairball = OnlyEnemyAt(world, world.player.motion.position + Vec2{200.0f, 0.0f});
    Step(world, AimRight(world, true), 1);
    for (int i = 0; i < 30 && hairball.health.current == hairball.health.max; ++i) {
        Step(world, AimRight(world, false), 1);
    }

    REQUIRE(hairball.health.current == hairball.health.max - horde::kPistol.damage);
    CHECK(hairball.hitFlashSeconds > 0.0f);
    CHECK(hairball.stunSeconds > 0.0f);
    // It was walking toward the player (-x). The knockback turns it around, away from the player.
    CHECK(hairball.motion.velocity.x > 0.0f);
}

TEST_CASE("a bullet hits only the nearest hairball in its path") {
    World world = PlayerAlone();
    const Vec2 player = world.player.motion.position;
    world.enemies.Spawn(Enemy{.motion = horde::MotionAt(player + Vec2{150.0f, 0.0f})});
    world.enemies.Spawn(Enemy{.motion = horde::MotionAt(player + Vec2{200.0f, 0.0f})});

    Step(world, AimRight(world, true), 1);
    Step(world, AimRight(world, false), 30);

    REQUIRE(world.enemies.Size() == 2);
    CHECK(world.enemies[0].health.current == world.enemies[0].health.max - horde::kPistol.damage);
    CHECK(world.enemies[1].health.current == world.enemies[1].health.max);
}

TEST_CASE("a hairball killed in a step is gone before it can touch the player") {
    World world = PlayerAlone();
    horde::Player& player = world.player;
    Enemy& hairball = OnlyEnemyAt(world, player.motion.position + Vec2{player.radius + 18.0f, 0.0f});
    hairball.health.current = 1;

    // Point blank: the muzzle is inside the hairball, so the bullet hits in the step it is fired.
    Step(world, AimRight(world, true), 1);
    CHECK(world.enemies.Empty());
    CHECK(player.health.current == player.health.max);
}

TEST_CASE("a dash carries the player its distance and leaves it at running speed") {
    World world = PlayerAlone();
    const Vec2 start = world.player.motion.position;
    Step(world, PlayerCommands{.move = {1.0f, 0.0f}, .dashPressed = true}, 1);
    Step(world, PlayerCommands{.move = {1.0f, 0.0f}}, 7);

    CHECK_FALSE(horde::IsDashing(world.player.dash));
    // The dash distance plus the little the player ran after it in the same eight steps.
    const float travelled = world.player.motion.position.x - start.x;
    CHECK(travelled >= world.player.dash.stats.distance);
    CHECK(travelled < world.player.dash.stats.distance + 10.0f);
    CHECK(world.player.motion.velocity.x == doctest::Approx(world.player.stats.maxSpeed));
}

TEST_CASE("a dash with no movement input goes where the player aims") {
    World world = PlayerAlone();
    const Vec2 start = world.player.motion.position;
    const Vec2 left = start + Vec2{-500.0f, 0.0f};
    Step(world, PlayerCommands{.aimAt = left, .dashPressed = true}, 1);
    Step(world, PlayerCommands{.aimAt = left}, 10);
    CHECK(world.player.motion.position.x < start.x - 150.0f);
    CHECK(world.player.motion.position.y == doctest::Approx(start.y));
}

TEST_CASE("a dash stops at a wall") {
    World world = PlayerAlone();
    world.player.motion = horde::MotionAt(Vec2{kArena.max.x - 60.0f, horde::Center(kArena).y});
    Step(world, PlayerCommands{.move = {1.0f, 0.0f}, .dashPressed = true}, 10);
    CHECK(world.player.motion.position.x == doctest::Approx(kArena.max.x - world.player.radius));
}

TEST_CASE("a dash goes through a hairball instead of shoving it") {
    World world = PlayerAlone();
    const Vec2 start = world.player.motion.position;
    Enemy& hairball = OnlyEnemyAt(world, start + Vec2{80.0f, 0.0f});
    hairball.stunSeconds = 1.0f; // Keep it in place, so only the dash moves anything.

    Step(world, PlayerCommands{.move = {1.0f, 0.0f}, .dashPressed = true}, 8);
    CHECK(world.player.motion.position.x > hairball.motion.position.x);
    CHECK(hairball.motion.position.x == doctest::Approx(start.x + 80.0f).epsilon(0.02));
}

TEST_CASE("a dashing player cannot be hurt") {
    World world = PlayerAlone();
    horde::Player& player = world.player;
    OnlyEnemyAt(world, player.motion.position + Vec2{player.radius + 18.0f, 0.0f});

    // Dash straight through the hairball: every step of the dash the bodies overlap or touch.
    Step(world, PlayerCommands{.move = {1.0f, 0.0f}, .dashPressed = true}, 1);
    CHECK(horde::IsDashInvulnerable(player.dash));
    CHECK(player.health.current == player.health.max);
}

TEST_CASE("the gun does not fire during a dash, but a click in it fires right after") {
    World world = PlayerAlone();
    Step(world, PlayerCommands{.move = {1.0f, 0.0f}, .dashPressed = true}, 1);
    Step(world, PlayerCommands{.move = {1.0f, 0.0f}, .firePressed = true}, 1);
    CHECK(world.shotsFired == 0);

    Step(world, PlayerCommands{.move = {1.0f, 0.0f}}, 7);
    CHECK(world.shotsFired == 1);
}

TEST_CASE("a dash with the cursor right on the player goes where the player faces") {
    World world = PlayerAlone();
    const Vec2 start = world.player.motion.position;
    // The player starts facing the boss, up the screen, and the cursor gives no direction.
    Step(world, PlayerCommands{.aimAt = start, .dashPressed = true}, 10);
    CHECK(world.player.motion.position.y < start.y - 150.0f);
}

TEST_CASE("a drink heals the player when it finishes and spends a flask") {
    World world = PlayerAlone();
    horde::Player& player = world.player;
    player.health.current = 50;

    Step(world, PlayerCommands{.healPressed = true}, 1);
    CHECK(horde::IsDrinking(player.flasks));
    CHECK(player.flasks.charges == player.flasks.stats.maxCharges - 1);
    Step(world, Vec2{}, 50);
    CHECK(player.health.current == 50);

    Step(world, Vec2{}, 10);
    CHECK(player.health.current == 50 + player.flasks.stats.heal);
}

TEST_CASE("a drink at full health is refused and keeps the flask") {
    World world = PlayerAlone();
    Step(world, PlayerCommands{.healPressed = true}, 1);
    CHECK_FALSE(horde::IsDrinking(world.player.flasks));
    CHECK(world.player.flasks.charges == world.player.flasks.stats.maxCharges);
}

TEST_CASE("while drinking the player is slow and cannot dash or shoot") {
    World world = PlayerAlone();
    horde::Player& player = world.player;
    player.health.current = 50;

    Step(world, PlayerCommands{.healPressed = true}, 1);
    Step(world, PlayerCommands{.move = {1.0f, 0.0f}, .firePressed = true, .dashPressed = true}, 1);
    Step(world, PlayerCommands{.move = {1.0f, 0.0f}}, 40);

    CHECK_FALSE(horde::IsDashing(player.dash));
    CHECK(world.shotsFired == 0);
    const float slowSpeed = player.stats.maxSpeed * player.flasks.stats.moveSpeedFactor;
    CHECK(player.motion.velocity.x == doctest::Approx(slowSpeed));
}

TEST_CASE("a hit while drinking wastes the flask") {
    World world = PlayerAlone();
    horde::Player& player = world.player;
    player.health.current = 50;
    Step(world, PlayerCommands{.healPressed = true}, 1);

    // A hairball touches the player in the middle of the drink.
    OnlyEnemyAt(world, player.motion.position + Vec2{player.radius + 18.0f, 0.0f});
    Step(world, Vec2{}, 1);
    CHECK_FALSE(horde::IsDrinking(player.flasks));
    CHECK(player.health.current == 40);

    world.enemies.Clear();
    Step(world, Vec2{}, 60);
    CHECK(player.health.current == 40);
    CHECK(player.flasks.charges == player.flasks.stats.maxCharges - 1);
}

TEST_CASE("the flasks run out after three drinks") {
    World world = PlayerAlone();
    horde::Player& player = world.player;
    for (int drink = 0; drink < 4; ++drink) {
        player.health.current = 10;
        Step(world, PlayerCommands{.healPressed = true}, 1);
        Step(world, Vec2{}, 61);
    }
    CHECK(player.flasks.charges == 0);
    CHECK(player.health.current == 10);
}
