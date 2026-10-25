#include <algorithm>
#include <cmath>
#include <limits>

#include <doctest/doctest.h>

#include "PredictableBoss.h"
#include "core/Collision.h"
#include "core/World.h"

using horde::AttackStage;
using horde::Boss;
using horde::Enemy;
using horde::PlayerCommands;
using horde::Projectile;
using horde::Vec2;
using horde::World;

namespace {

constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

// A fight against a boss without its chance, so each attack plays at its stats' exact timings.
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

// The boss in the middle, starting a Hair Ring on the next step, with the player standing `offset` away
// from it. The boss is held in place, so it does not walk into the player once the ring is over.
World AboutToSpit(Vec2 offset) {
    World world = NewFight();
    world.player.motion = horde::MotionAt(world.boss.motion.position + offset);
    world.boss.nextAttack = horde::AttackKind::HairRing;
    world.boss.secondsUntilAttack = 0.0f;
    world.boss.stats.maxSpeed = 0.0f;
    return world;
}

// The boss in the middle, starting a Leap on the next step, with the player standing `offset` away. Its
// shadow is pinned where the player starts, so the test knows where the boss comes down: `Landing`.
World AboutToLeap(Vec2 offset) {
    World world = NewFight();
    world.player.motion = horde::MotionAt(world.boss.motion.position + offset);
    world.boss.nextAttack = horde::AttackKind::Leap;
    world.boss.secondsUntilAttack = 0.0f;
    world.boss.leap.shadowSpeed = 0.0f;
    return world;
}

Vec2 Landing(const World& world) {
    REQUIRE(world.boss.attack);
    return world.boss.attack->target;
}

// Plays a Leap to its landing with the player running east, starting `delay` seconds after the boss
// leaves the ground. The shadow follows, as in the game.
void LeapRunningEast(World& world, float delay) {
    world.boss.leap.shadowSpeed = horde::LeapStats{}.shadowSpeed;
    Step(world, PlayerCommands{}, 1);
    float airborne = 0.0f;
    while (world.boss.attack && world.boss.attack->stage == AttackStage::Warning) {
        const bool run = horde::IsAirborne(world.boss) && airborne >= delay;
        Step(world, PlayerCommands{.move = run ? Vec2{1.0f, 0.0f} : Vec2{}}, 1);
        airborne += horde::IsAirborne(world.boss) ? kDt : 0.0f;
    }
}

// How far the shockwave's front is from reaching the player. Negative once it has.
float ShockwaveGap(const World& world) {
    REQUIRE(world.shockwaves.Size() == 1);
    const horde::Shockwave& wave = world.shockwaves[0];
    return Distance(wave.center, world.player.motion.position) - world.player.radius - wave.radius;
}

// How far the strand nearest to the player is from touching them. Negative once it overlaps.
float NearestStrandGap(const World& world) {
    float nearest = std::numeric_limits<float>::infinity();
    for (const Projectile& strand : world.hostileProjectiles) {
        const float gap = Distance(strand.motion.position, world.player.motion.position) - strand.radius -
                          world.player.radius;
        nearest = std::min(nearest, gap);
    }
    return nearest;
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
    REQUIRE_FALSE(world.lastHitBy);
    Step(world, PlayerCommands{}, 1);
    CHECK(world.player.health.current == world.player.health.max - world.boss.contactDamage);
    CHECK(world.lastHitBy == horde::DamageSource::BossTouch);
}

TEST_CASE("when the boss and a hairball touch the player in the same step, the boss's hit lands") {
    World world = NewFight();
    PlayerTouchingBoss(world);
    const float hairballContact = world.player.radius + Enemy{}.radius;
    world.enemies.Spawn(Enemy{.motion = horde::MotionAt(world.player.motion.position + Vec2{hairballContact, 0.0f})});

    Step(world, PlayerCommands{}, 1);
    CHECK(world.player.health.current == world.player.health.max - world.boss.contactDamage);
}

TEST_CASE("with its shield down, a bullet takes health from the boss and flashes it, but does not push it") {
    World world = NewFight();
    const Boss& boss = world.boss;
    horde::BreakShield(world.boss.shield);
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

TEST_CASE("a shield that is up takes the bullets instead of the boss") {
    World world = NewFight();
    const Boss& boss = world.boss;
    const float full = boss.shield.strength;
    // The player starts aiming at the boss with the pistol: the shell takes the shot.
    Step(world, PlayerCommands{.firePressed = true}, 1);
    for (int i = 0; i < 30 && boss.shield.strength == full; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    CHECK(boss.shield.strength == doctest::Approx(full - static_cast<float>(horde::kPistol.damage)));
    CHECK(boss.health.current == boss.health.max);
    CHECK(boss.hitFlashSeconds == 0.0f);
    CHECK(world.shotsHit == 1);
}

TEST_CASE("SMG bullets bounce off the shell without a scratch") {
    World world = NewFight();
    const Boss& boss = world.boss;
    Step(world, PlayerCommands{.selectSlot = 1}, 1);
    Step(world, PlayerCommands{.fireHeld = true, .firePressed = true}, 40);
    Step(world, PlayerCommands{}, 30);
    CHECK(world.shotsHit > 3);
    CHECK(boss.shield.strength == doctest::Approx(boss.shield.stats.shellStrength));
    CHECK(boss.health.current == boss.health.max);
}

TEST_CASE("the bullet that breaks the shield staggers the boss: its attack is cut off and it holds still") {
    World world = AboutToCharge(250.0f);
    Boss& boss = world.boss;
    boss.shield.strength = static_cast<float>(horde::kPistol.damage);
    StepUntilStage(world, PlayerCommands{}, AttackStage::Warning);
    Step(world, PlayerCommands{.firePressed = true}, 1);
    for (int i = 0; i < 30 && horde::IsShieldUp(boss.shield); ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    REQUIRE_FALSE(horde::IsShieldUp(boss.shield));
    CHECK(horde::IsStaggered(boss));
    CHECK_FALSE(boss.attack);
    CHECK(boss.health.current == boss.health.max);
    const Vec2 where = boss.motion.position;

    // A second of stagger: no attack and no walking, then it stalks again.
    Step(world, PlayerCommands{}, 55);
    CHECK(horde::IsStaggered(boss));
    CHECK_FALSE(boss.attack);
    CHECK(Distance(boss.motion.position, where) < 1.0f);
    Step(world, PlayerCommands{}, 10);
    CHECK_FALSE(horde::IsStaggered(boss));
    Step(world, PlayerCommands{}, 20);
    CHECK(Distance(boss.motion.position, where) > 1.0f);
}

TEST_CASE("the Debug shortcut breaks the shield and staggers the boss like a real break") {
    World world = NewFight();
    horde::BreakBossShield(world);
    CHECK_FALSE(horde::IsShieldUp(world.boss.shield));
    CHECK(horde::IsStaggered(world.boss));
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
    CHECK(world.lastHitBy == horde::DamageSource::Charge);
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
    CHECK_FALSE(world.lastHitBy);
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

TEST_CASE("the strands that reach the player take one strand's damage, and are spent even inside the window") {
    // The gap opens to the side, so the ring comes straight at a player 200 units south. The two strands
    // either side of due south both reach the player in the same step: the first one hurts, and the
    // invulnerability window after it lets the second one hurt nothing, but it is spent all the same.
    World world = AboutToSpit(Vec2{0.0f, 200.0f});
    const horde::Player& player = world.player;
    for (int i = 0; i < 180 && player.health.current == player.health.max; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    const horde::HairRingStats& ring = world.boss.hairRing;
    CHECK(player.health.current == player.health.max - ring.damage);
    CHECK(world.lastHitBy == horde::DamageSource::HairRing);
    CHECK(world.hostileProjectiles.Size() == static_cast<std::size_t>(horde::HairRingStrands(ring) - 2));
}

TEST_CASE("standing in the gap, the whole ring flies past") {
    World world = AboutToSpit(Vec2{0.0f, 200.0f});
    Step(world, PlayerCommands{}, 1);
    REQUIRE(world.boss.attack);
    const Vec2 boss = world.boss.motion.position;
    world.player.motion = horde::MotionAt(boss + world.boss.attack->direction * 200.0f);

    // The ring reaches 200 units about 1.4 s after the warning starts, and is past the player by 2 s.
    Step(world, PlayerCommands{}, 120);
    CHECK(world.player.health.current == world.player.health.max);
    REQUIRE(world.hostileProjectiles.Size() == static_cast<std::size_t>(horde::HairRingStrands(world.boss.hairRing)));
    CHECK(Distance(world.hostileProjectiles[0].motion.position, boss) > 200.0f + world.player.radius);
}

TEST_CASE("a dash through the ring takes no damage, and the strands fly on") {
    World world = AboutToSpit(Vec2{0.0f, 400.0f});
    for (int i = 0; i < 180 && NearestStrandGap(world) > 10.0f; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    REQUIRE(NearestStrandGap(world) <= 10.0f);

    // Toward the boss, through the ring.
    Step(world, PlayerCommands{.move = {0.0f, -1.0f}, .dashPressed = true}, 1);
    Step(world, PlayerCommands{}, 20);
    CHECK(world.player.health.current == world.player.health.max);
    CHECK(world.hostileProjectiles.Size() == static_cast<std::size_t>(horde::HairRingStrands(world.boss.hairRing)));
    CHECK(world.player.motion.position.y < world.boss.motion.position.y + 250.0f);
}

TEST_CASE("the boss's strands hurt only the player: hairballs, the boss and bullets let them through") {
    World world = NewFight();
    OnlyWalks(world.boss);
    const Vec2 boss = world.boss.motion.position;
    const Enemy& hairball =
        *world.enemies.Spawn(Enemy{.motion = horde::MotionAt(boss + Vec2{-300.0f, 0.0f}), .stunSeconds = 1.0f});
    // One strand leaving the boss's body, one inside the hairball, and one head on with a bullet.
    world.hostileProjectiles.Spawn(horde::MakeHairStrand(world.boss, Vec2{1.0f, 0.0f}));
    world.hostileProjectiles.Spawn(horde::MakeHairStrand(world.boss, Vec2{-1.0f, 0.0f}));
    world.hostileProjectiles[1].motion = horde::MotionAt(hairball.motion.position);
    world.hostileProjectiles[1].motion.velocity = Vec2{-world.boss.hairRing.speed, 0.0f};
    const Vec2 crossing = boss + Vec2{300.0f, -200.0f};
    world.hostileProjectiles.Spawn(horde::MakeHairStrand(world.boss, Vec2{-1.0f, 0.0f}));
    world.hostileProjectiles[2].motion = horde::MotionAt(crossing);
    world.hostileProjectiles[2].motion.velocity = Vec2{-world.boss.hairRing.speed, 0.0f};
    world.bullets.Spawn(horde::MakeBullet(crossing, Vec2{1.0f, 0.0f}, horde::kPistol));

    Step(world, PlayerCommands{}, 1);
    CHECK(hairball.health.current == hairball.health.max);
    CHECK(world.boss.health.current == world.boss.health.max);
    CHECK(world.hostileProjectiles.Size() == 3);
    CHECK(world.bullets.Size() == 1);
}

TEST_CASE("a strand that reaches a wall is gone at once") {
    World world = NewFight();
    OnlyWalks(world.boss);
    world.hostileProjectiles.Spawn(horde::LaunchProjectile(Vec2{world.arena.max.x - 2.0f, 200.0f}, Vec2{1.0f, 0.0f},
                                                           world.boss.hairRing.speed, world.boss.hairRing.range));
    Step(world, PlayerCommands{}, 1);
    CHECK(world.hostileProjectiles.Empty());
}

TEST_CASE("landing on the player takes the impact's damage once and throws them out from under the boss") {
    World world = AboutToLeap(Vec2{0.0f, 200.0f});
    const horde::Player& player = world.player;
    for (int i = 0; i < 120 && player.health.current == player.health.max; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    const horde::LeapStats& leap = world.boss.leap;
    REQUIRE(player.health.current == player.health.max - leap.impactDamage);
    CHECK(world.lastHitBy == horde::DamageSource::LeapImpact);
    CHECK(horde::Length(player.motion.velocity) > leap.impactKnockback * 0.5f);

    // The shockwave leaves through the player inside the window after the impact: one landing, one hit.
    Step(world, PlayerCommands{}, 30);
    CHECK(player.health.current == player.health.max - leap.impactDamage);
    CHECK(Distance(player.motion.position, world.boss.motion.position) >= world.boss.radius + player.radius - 0.1f);
}

TEST_CASE("in the air the boss is not a body: it does not block, take bullets or hurt") {
    World world = AboutToLeap(Vec2{0.0f, 200.0f});
    for (int i = 0; i < 60 && !horde::IsAirborne(world.boss); ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    REQUIRE(horde::IsAirborne(world.boss));

    // The player stands right under the flying boss and shoots straight up through it.
    world.player.motion = horde::MotionAt(world.boss.motion.position);
    world.bullets.Spawn(horde::MakeBullet(world.boss.motion.position + Vec2{0.0f, 100.0f}, Vec2{0.0f, -1.0f},
                                          horde::kPistol));
    const Vec2 under = world.player.motion.position;
    Step(world, PlayerCommands{}, 10);
    REQUIRE(horde::IsAirborne(world.boss));
    CHECK(world.player.motion.position == under);
    CHECK(world.player.health.current == world.player.health.max);
    CHECK(world.boss.health.current == world.boss.health.max);
}

TEST_CASE("the shockwave hurts a player outside the landing zone once, when its band arrives") {
    World world = AboutToLeap(Vec2{0.0f, 200.0f});
    Step(world, PlayerCommands{}, 1);
    world.player.motion = horde::MotionAt(Landing(world) + Vec2{220.0f, 0.0f});
    const horde::Player& player = world.player;
    for (int i = 0; i < 180 && player.health.current == player.health.max; ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    CHECK(player.health.current == player.health.max - world.boss.leap.shockwave.damage);
    CHECK(world.lastHitBy == horde::DamageSource::Shockwave);
    // Long after the window has closed, the same wave cannot hurt again.
    Step(world, PlayerCommands{}, 60);
    CHECK(player.health.current == player.health.max - world.boss.leap.shockwave.damage);
}

TEST_CASE("a dash through the shockwave takes no damage") {
    World world = AboutToLeap(Vec2{0.0f, 200.0f});
    Step(world, PlayerCommands{}, 1);
    world.player.motion = horde::MotionAt(Landing(world) + Vec2{350.0f, 0.0f});
    for (int i = 0; i < 180 && (world.shockwaves.Empty() || ShockwaveGap(world) > 8.0f); ++i) {
        Step(world, PlayerCommands{}, 1);
    }
    REQUIRE(ShockwaveGap(world) <= 8.0f);

    // Toward the landing point, through the band.
    Step(world, PlayerCommands{.move = {-1.0f, 0.0f}, .dashPressed = true}, 1);
    Step(world, PlayerCommands{}, 40);
    CHECK(world.player.health.current == world.player.health.max);
}

TEST_CASE("beyond the shockwave's reach the landing is harmless") {
    World world = AboutToLeap(Vec2{0.0f, 200.0f});
    Step(world, PlayerCommands{}, 1);
    world.player.motion = horde::MotionAt(Landing(world) + Vec2{500.0f, 0.0f});
    world.boss.stats.maxSpeed = 0.0f; // So it does not walk over once the leap is done.
    Step(world, PlayerCommands{}, 180);
    CHECK(world.shockwaves.Empty());
    CHECK(world.player.health.current == world.player.health.max);
}

TEST_CASE("running from the takeoff escapes the landing, but starting only when the shadow locks does not") {
    World early = AboutToLeap(Vec2{0.0f, 200.0f});
    LeapRunningEast(early, 0.0f);
    CHECK(early.player.health.current == early.player.health.max);

    World late = AboutToLeap(Vec2{0.0f, 200.0f});
    const horde::LeapStats& leap = late.boss.leap;
    LeapRunningEast(late, leap.airSeconds - leap.lockSeconds);
    CHECK(late.player.health.current == late.player.health.max - leap.impactDamage);
}
