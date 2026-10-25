#include <algorithm>
#include <cmath>
#include <vector>

#include <doctest/doctest.h>

#include "PredictableBoss.h"
#include "core/Boss.h"
#include "core/World.h"

using horde::AttackKind;
using horde::AttackStage;
using horde::Boss;
using horde::BossAttack;
using horde::PlayerCommands;
using horde::Projectile;
using horde::Vec2;

namespace {

// The game's arena: (0, 0) to (1640, 1040), centre (820, 520).
constexpr horde::Arena kArena = horde::kBossArena;
constexpr Vec2 kCenter{820.0f, 520.0f};
constexpr float kDt = 1.0f / 60.0f;
constexpr float kPlayerRadius = 20.0f;
constexpr Vec2 kView{1280.0f, 720.0f};

// What the bosses below draw from. With their chance taken out, it only decides the stalks between attacks.
horde::Random chance = horde::SeedRandom(4);

// A boss without its chance in the middle, at the very start of a Volley's warning.
Boss AboutToVolley() {
    Boss boss = horde::test::Predictable(Boss{});
    boss.motion = horde::MotionAt(kCenter);
    boss.attack = BossAttack{.kind = AttackKind::Volley};
    return boss;
}

// Steps the boss, and returns how many bursts it fired.
int Step(Boss& boss, Vec2 player, int steps) {
    int bursts = 0;
    for (int i = 0; i < steps; ++i) {
        bursts += horde::StepBoss(boss, horde::MotionAt(player), kPlayerRadius, kArena, chance, kDt).spitVolley ? 1 : 0;
    }
    return bursts;
}

} // namespace

TEST_CASE("during the volley's warning the boss holds still and its aim follows the player") {
    Boss boss = AboutToVolley();
    Step(boss, kCenter + Vec2{300.0f, 0.0f}, 10);
    REQUIRE(boss.attack);
    CHECK(boss.attack->stage == AttackStage::Warning);
    CHECK(boss.attack->direction.x == doctest::Approx(1.0f));
    Step(boss, kCenter + Vec2{0.0f, -300.0f}, 10);
    CHECK(boss.attack->direction.y == doctest::Approx(-1.0f));
    CHECK(boss.motion.position == kCenter);
}

TEST_CASE("a volley fires three bursts a beat apart, each aimed where the player is when it leaves") {
    Boss boss = AboutToVolley();
    // The player moves around the boss: east, then south, then west.
    const Vec2 places[] = {kCenter + Vec2{300.0f, 0.0f}, kCenter + Vec2{0.0f, 300.0f}, kCenter + Vec2{-300.0f, 0.0f}};
    std::vector<int> burstSteps;
    std::vector<Vec2> burstAims;
    for (int i = 0; i < 300 && boss.attack; ++i) {
        const Vec2 player = places[burstSteps.size() % 3];
        if (Step(boss, player, 1) == 1) {
            burstSteps.push_back(i);
            burstAims.push_back(boss.attack->direction);
        }
    }
    REQUIRE(burstSteps.size() == 3);
    // The first one as the 0.55 s warning ends, then 0.3 s apart.
    CHECK(burstSteps[0] == doctest::Approx(34).epsilon(0.05));
    CHECK(burstSteps[1] - burstSteps[0] == doctest::Approx(18).epsilon(0.1));
    CHECK(burstSteps[2] - burstSteps[1] == doctest::Approx(18).epsilon(0.1));
    CHECK(burstAims[0].x == doctest::Approx(1.0f));
    CHECK(burstAims[1].y == doctest::Approx(1.0f));
    CHECK(burstAims[2].x == doctest::Approx(-1.0f));
}

TEST_CASE("after the last burst the boss recovers, then stalks again") {
    Boss boss = AboutToVolley();
    const Vec2 player = kCenter + Vec2{300.0f, 0.0f};
    int bursts = 0;
    while (bursts < 3) {
        bursts += Step(boss, player, 1);
    }
    // 0.15 s more of the active stage and 0.8 s of recovery.
    int steps = 0;
    while (boss.attack && steps < 120) {
        Step(boss, player, 1);
        ++steps;
    }
    CHECK(steps == doctest::Approx(57).epsilon(0.05));
    CHECK_FALSE(boss.attack);
    CHECK(boss.secondsUntilAttack >= boss.choice.minStalkSeconds);
    CHECK(boss.secondsUntilAttack <= boss.choice.maxStalkSeconds);
}

TEST_CASE("a burst is five strands fanned around the aim, leaving the edge of the body at the volley's speed") {
    horde::World world = horde::CreateWorld(kArena, kView);
    world.boss = AboutToVolley();
    world.boss.motion = horde::MotionAt(kCenter);
    world.player.motion = horde::MotionAt(kCenter + Vec2{0.0f, 400.0f});
    for (int i = 0; i < 60 && world.hostileProjectiles.Size() == 0; ++i) {
        horde::StepWorld(world, PlayerCommands{}, kDt);
    }
    REQUIRE(world.hostileProjectiles.Size() == 5);
    const horde::VolleyStats& stats = world.boss.volley;
    std::vector<float> angles;
    for (const Projectile& strand : world.hostileProjectiles) {
        CHECK(horde::Length(strand.motion.velocity) == doctest::Approx(stats.speed));
        CHECK(strand.damage == stats.damage);
        CHECK(strand.source == horde::DamageSource::Volley);
        // Strands move one step before the test sees them, so they are a little past the edge of the body.
        const float out = horde::Length(strand.motion.position - kCenter);
        CHECK(out >= world.boss.radius);
        CHECK(out <= world.boss.radius + stats.speed * kDt + 0.01f);
        angles.push_back(std::atan2(strand.motion.velocity.x, strand.motion.velocity.y) * 180.0f / 3.14159265f);
    }
    // Straight south (0 degrees here), and 8 and 16 degrees either side.
    float lowest = angles[0];
    float highest = angles[0];
    for (const float angle : angles) {
        lowest = std::min(lowest, angle);
        highest = std::max(highest, angle);
    }
    CHECK(lowest == doctest::Approx(-16.0f).epsilon(0.01));
    CHECK(highest == doctest::Approx(16.0f).epsilon(0.01));
}

TEST_CASE("a volley strand that hits the player counts as the volley's") {
    horde::World world = horde::CreateWorld(kArena, kView);
    world.boss = AboutToVolley();
    world.boss.motion = horde::MotionAt(kCenter);
    world.player.motion = horde::MotionAt(kCenter + Vec2{0.0f, 250.0f});
    const horde::Player& player = world.player;
    for (int i = 0; i < 120 && player.health.current == player.health.max; ++i) {
        horde::StepWorld(world, PlayerCommands{}, kDt);
    }
    CHECK(player.health.current == player.health.max - world.boss.volley.damage);
    CHECK(world.lastHitBy == horde::DamageSource::Volley);
}

TEST_CASE("the second burst leads a running player, the first and the third aim at them") {
    Boss boss = AboutToVolley();
    // The player runs east at full speed, 300 units south of the boss.
    horde::Motion player = horde::MotionAt(kCenter + Vec2{-200.0f, 300.0f});
    player.velocity = Vec2{300.0f, 0.0f};
    std::vector<Vec2> aims;
    std::vector<horde::Motion> players;
    for (int i = 0; i < 300 && boss.attack; ++i) {
        player.position += player.velocity * kDt;
        if (horde::StepBoss(boss, player, kPlayerRadius, kArena, chance, kDt).spitVolley) {
            aims.push_back(boss.attack->direction);
            players.push_back(player);
        }
    }
    REQUIRE(aims.size() == 3);
    for (const std::size_t aimed : {std::size_t{0}, std::size_t{2}}) {
        const Vec2 atPlayer = horde::Normalized(players[aimed].position - kCenter);
        CHECK(aims[aimed].x == doctest::Approx(atPlayer.x).epsilon(0.001));
        CHECK(aims[aimed].y == doctest::Approx(atPlayer.y).epsilon(0.001));
    }
    // The led one points well ahead of the player, and its middle strand, leaving the edge of the body at the
    // Volley's speed, meets a player who keeps running.
    const Vec2 atPlayer = horde::Normalized(players[1].position - kCenter);
    CHECK(aims[1].x > atPlayer.x + 0.2f);
    float closest = 1.0e9f;
    for (int i = 0; i < 180; ++i) {
        const float t = static_cast<float>(i) * kDt;
        const Vec2 strand = kCenter + aims[1] * (boss.radius + boss.volley.speed * t);
        closest = std::min(closest, horde::Length(strand - (players[1].position + players[1].velocity * t)));
    }
    CHECK(closest < boss.volley.strandRadius + kPlayerRadius);
}

TEST_CASE("a led burst looks ahead at most its cap, and never past the walls") {
    Boss boss = AboutToVolley();
    boss.volley.speed = 50.0f; // A flight of seconds, far above the one second cap.
    // Near the east wall, running east.
    horde::Motion player = horde::MotionAt(Vec2{kArena.max.x - 30.0f, kCenter.y + 300.0f});
    player.velocity = Vec2{300.0f, 0.0f};
    int bursts = 0;
    while (bursts < 2) {
        bursts += horde::StepBoss(boss, player, kPlayerRadius, kArena, chance, kDt).spitVolley ? 1 : 0;
    }
    // Led to the wall, not 300 units past it.
    const Vec2 wall{kArena.max.x, player.position.y};
    const Vec2 expected = horde::Normalized(wall - kCenter);
    CHECK(boss.attack->direction.x == doctest::Approx(expected.x).epsilon(0.001));
    CHECK(boss.attack->direction.y == doctest::Approx(expected.y).epsilon(0.001));
}

TEST_CASE("a player who runs in a straight line through a whole volley is hit, across or away") {
    struct Run {
        Vec2 start;
        Vec2 move;
    };
    // Across the boss's aim, and away from the boss on a diagonal, from far and from closer.
    const Run runs[] = {{{-450.0f, 350.0f}, {1.0f, 0.0f}},
                        {{-100.0f, 300.0f}, {-1.0f, 0.0f}},
                        {{200.0f, 250.0f}, horde::Normalized(Vec2{1.0f, 1.0f})},
                        {{0.0f, 450.0f}, {1.0f, 0.0f}}};
    for (const Run& run : runs) {
        horde::World world = horde::CreateWorld(kArena, kView);
        world.boss = AboutToVolley();
        world.boss.motion = horde::MotionAt(kCenter);
        world.boss.stats.maxSpeed = 0.0f;
        world.player.motion = horde::MotionAt(kCenter + run.start);
        const horde::Player& player = world.player;
        for (int i = 0; i < 150 && player.health.current == player.health.max; ++i) {
            horde::StepWorld(world, PlayerCommands{.move = run.move}, kDt);
        }
        CHECK(player.health.current == player.health.max - world.boss.volley.damage);
        CHECK(world.lastHitBy == horde::DamageSource::Volley);
    }
}
