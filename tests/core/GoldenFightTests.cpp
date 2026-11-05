#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/Boss.h"
#include "core/Collision.h"
#include "core/World.h"

// The golden fight: one whole fight against the Matriarch with a fixed seed and a scripted hunter, sampled
// once a simulated second and compared with a table recorded from the game as it was. It exists to guard
// refactors of the boss: they must not change what she does, and any change in her timing, her choices or
// what she draws from the random generator shows up here as the first second that differs.
//
// When a change is meant to alter the fight (new numbers, a new attack), record the table again and say why
// in the commit:
//
//   build/default/tests/Release/horde_tests.exe --no-skip --minimal -tc="golden fight: print the table"
//
// and paste the rows between the BEGIN and END lines into GoldenFightTable.inc.

using horde::PlayerCommands;
using horde::Vec2;
using horde::World;

namespace {

constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;
constexpr int kStepsPerSecond = 60;
constexpr int kSeconds = 120;
constexpr std::uint64_t kSeed = 20261020; // The day the project started.

// The hunter cannot die, so the fight runs its whole length and reaches the second phase. Its hits still
// land and push it around like a real player's.
constexpr int kSturdyHealth = 1000000;

// The distance it keeps, changed every few seconds through her near, mid and far ranges, so every attack
// gets picked: the Summon only comes for a player who keeps far away.
constexpr float kKeepDistances[] = {300.0f, 450.0f, 600.0f};
constexpr int kKeepDistanceSteps = 8 * 60;
constexpr int kStrafeTurnSteps = 6 * 60;       // Circles one way, then the other.
constexpr int kPistolPressSteps = 10;          // The pistol's 6 shots a second.
constexpr float kDodgeDistance = 80.0f;        // A strand this close is dashed through.
constexpr float kChargeDodgeDistance = 200.0f;
// Closer than this to a wall it heads back to the middle, or the boss pins it in a corner for the rest of
// the fight and the table stops describing a fight.
constexpr float kWallMargin = 160.0f;

// What is recorded each second. Ints for everything that is counted or picked, floats only for positions
// and the shield, and the generator's state, which catches a single extra or missing draw.
struct GoldenSample {
    int second;
    int phase;  ///< `BossPhase`.
    int frenzied;
    int attack; ///< `AttackKind`, or -1 while she stalks.
    int stage;  ///< `AttackStage`, or -1 while she stalks.
    int shieldKind;
    int shieldUp;
    float shieldStrength;
    int bossHealth;
    float bossX;
    float bossY;
    int playerHealth;
    float playerX;
    float playerY;
    int hairballs;
    int hostileProjectiles;
    int shotsFired;
    int shotsHit;
    std::uint64_t randomState;
};

constexpr GoldenSample kGolden[] = {
#include "GoldenFightTable.inc"
};

// Positions and strengths are compared to a hundredth of a unit: a refactor that keeps the rules keeps
// these exact, and the table is printed with three decimals.
constexpr float kTolerance = 0.01f;

float NearestHostileDistance(const World& world) {
    float nearest = 1.0e9f;
    for (const horde::Projectile& strand : world.hostileProjectiles) {
        const float gap = horde::Length(strand.motion.position - world.player.motion.position) - strand.radius -
                          world.player.radius;
        nearest = std::min(nearest, gap);
    }
    return nearest;
}

// A plain twin-stick player: circles the boss at a distance that changes now and then, turns back now and
// then, keeps off the walls, aims at her, fires all the time with the gun her shield needs, and dashes
// through strands and charges that come close.
PlayerCommands Hunter(const World& world, int step) {
    const Vec2 player = world.player.motion.position;
    const Vec2 boss = world.boss.motion.position;
    const float distance = horde::Length(boss - player);
    const Vec2 toBoss = distance > 0.0f ? (boss - player) * (1.0f / distance) : Vec2{0.0f, -1.0f};
    const float side = (step / kStrafeTurnSteps) % 2 == 0 ? 1.0f : -1.0f;
    const Vec2 around = Vec2{-toBoss.y, toBoss.x} * side;
    const float keep = kKeepDistances[(step / kKeepDistanceSteps) % std::size(kKeepDistances)];
    const float closer = std::clamp((distance - keep) / 100.0f, -1.0f, 1.0f);
    Vec2 move = around + toBoss * closer;

    const horde::Arena& arena = world.arena;
    const float wallGap = std::min({player.x - arena.min.x, arena.max.x - player.x, player.y - arena.min.y,
                                    arena.max.y - player.y});
    if (wallGap < kWallMargin) {
        const Vec2 toMiddle = (arena.min + arena.max) * 0.5f - player;
        move += toMiddle * (2.0f / horde::Length(toMiddle));
    }

    const horde::Shield& shield = world.boss.shield;
    const bool needsPistol = horde::IsShieldUp(shield) && shield.kind == horde::ShieldKind::Shell;
    const bool dodge = NearestHostileDistance(world) < kDodgeDistance ||
                       (horde::IsCharging(world.boss) && distance < kChargeDodgeDistance);

    return PlayerCommands{.move = move,
                          .aimAt = boss,
                          .fireHeld = true,
                          .firePressed = step % kPistolPressSteps == 0,
                          .selectSlot = needsPistol ? 0 : 1,
                          .dashPressed = dodge};
}

GoldenSample Sample(const World& world, int second) {
    const horde::Boss& boss = world.boss;
    return GoldenSample{
        .second = second,
        .phase = static_cast<int>(boss.phase),
        .frenzied = boss.frenzied ? 1 : 0,
        .attack = boss.attack ? static_cast<int>(boss.attack->kind) : -1,
        .stage = boss.attack ? static_cast<int>(boss.attack->clock.stage) : -1,
        .shieldKind = static_cast<int>(boss.shield.kind),
        .shieldUp = horde::IsShieldUp(boss.shield) ? 1 : 0,
        .shieldStrength = boss.shield.strength,
        .bossHealth = boss.health.current,
        .bossX = boss.motion.position.x,
        .bossY = boss.motion.position.y,
        .playerHealth = world.player.health.current,
        .playerX = world.player.motion.position.x,
        .playerY = world.player.motion.position.y,
        .hairballs = static_cast<int>(world.enemies.Size()),
        .hostileProjectiles = static_cast<int>(world.hostileProjectiles.Size()),
        .shotsFired = world.shotsFired,
        .shotsHit = world.shotsHit,
        .randomState = world.random.state,
    };
}

// What the scenario went through, checked so a later re-recording cannot quietly stop covering it.
struct Coverage {
    bool attackSeen[5] = {};
    bool reachedPhaseTwo = false;
    bool frenzied = false;
    int shieldBreaks = 0;
    int hitsTaken = 0;
    int mostHairballs = 0;
    bool won = false;
};

struct GoldenRun {
    std::vector<GoldenSample> samples;
    Coverage coverage;
};

GoldenRun RunGoldenFight() {
    World world = horde::CreateWorld(horde::kBossArena, kView, kSeed);
    world.player.health.max = kSturdyHealth;
    world.player.health.current = kSturdyHealth;

    GoldenRun run;
    Coverage& coverage = run.coverage;
    for (int step = 0; step < kSeconds * kStepsPerSecond; ++step) {
        const bool shieldWasUp = horde::IsShieldUp(world.boss.shield);
        const int healthBefore = world.player.health.current;

        horde::StepWorld(world, Hunter(world, step), kDt);
        world.events.Clear(); // As the game does after reading them.

        if (world.boss.attack) {
            coverage.attackSeen[static_cast<int>(world.boss.attack->kind)] = true;
        }
        coverage.reachedPhaseTwo = coverage.reachedPhaseTwo || world.boss.phase == horde::BossPhase::Two;
        coverage.frenzied = coverage.frenzied || world.boss.frenzied;
        coverage.shieldBreaks += shieldWasUp && !horde::IsShieldUp(world.boss.shield) ? 1 : 0;
        coverage.hitsTaken += world.player.health.current < healthBefore ? 1 : 0;
        coverage.mostHairballs = std::max(coverage.mostHairballs, static_cast<int>(world.enemies.Size()));
        coverage.won = coverage.won || horde::IsBossDefeated(world);

        if ((step + 1) % kStepsPerSecond == 0) {
            run.samples.push_back(Sample(world, (step + 1) / kStepsPerSecond));
        }
    }
    return run;
}

bool Near(float a, float b) {
    return std::fabs(a - b) <= kTolerance;
}

// Every field that differs between two samples, or an empty string when they match.
std::string Differences(const GoldenSample& expected, const GoldenSample& actual) {
    std::ostringstream out;
    const auto exact = [&out](const char* name, auto want, auto got) {
        if (want != got) {
            out << " " << name << " " << want << " -> " << got << ";";
        }
    };
    const auto near = [&out](const char* name, float want, float got) {
        if (!Near(want, got)) {
            out << " " << name << " " << want << " -> " << got << ";";
        }
    };
    exact("phase", expected.phase, actual.phase);
    exact("frenzied", expected.frenzied, actual.frenzied);
    exact("attack", expected.attack, actual.attack);
    exact("stage", expected.stage, actual.stage);
    exact("shieldKind", expected.shieldKind, actual.shieldKind);
    exact("shieldUp", expected.shieldUp, actual.shieldUp);
    near("shieldStrength", expected.shieldStrength, actual.shieldStrength);
    exact("bossHealth", expected.bossHealth, actual.bossHealth);
    near("bossX", expected.bossX, actual.bossX);
    near("bossY", expected.bossY, actual.bossY);
    exact("playerHealth", expected.playerHealth, actual.playerHealth);
    near("playerX", expected.playerX, actual.playerX);
    near("playerY", expected.playerY, actual.playerY);
    exact("hairballs", expected.hairballs, actual.hairballs);
    exact("hostileProjectiles", expected.hostileProjectiles, actual.hostileProjectiles);
    exact("shotsFired", expected.shotsFired, actual.shotsFired);
    exact("shotsHit", expected.shotsHit, actual.shotsHit);
    exact("randomState", expected.randomState, actual.randomState);
    return out.str();
}

} // namespace

TEST_CASE("golden fight: the matriarch fights exactly as she did when the table was recorded") {
    const GoldenRun run = RunGoldenFight();
    REQUIRE(run.samples.size() == std::size(kGolden));

    for (std::size_t i = 0; i < run.samples.size(); ++i) {
        const std::string differences = Differences(kGolden[i], run.samples[i]);
        if (!differences.empty()) {
            // Only the first second that differs: everything after it differs too.
            FAIL("first difference at " << kGolden[i].second << " s:" << differences);
        }
    }
}

TEST_CASE("golden fight: the scenario covers every attack, both phases, the frenzy, the shield and the win") {
    const Coverage coverage = RunGoldenFight().coverage;
    for (int kind = 0; kind < 5; ++kind) {
        CAPTURE(kind);
        CHECK(coverage.attackSeen[kind]);
    }
    CHECK(coverage.reachedPhaseTwo);
    CHECK(coverage.frenzied);
    CHECK(coverage.shieldBreaks >= 4);
    CHECK(coverage.hitsTaken > 0);
    CHECK(coverage.mostHairballs > 0);
    CHECK(coverage.won);
}

TEST_CASE("golden fight: print the table" * doctest::skip()) {
    const GoldenRun run = RunGoldenFight();
    std::printf("BEGIN\n");
    for (const GoldenSample& s : run.samples) {
        std::printf("{%d, %d, %d, %d, %d, %d, %d, %.3ff, %d, %.3ff, %.3ff, %d, %.3ff, %.3ff, %d, %d, %d, %d, "
                    "0x%016llxULL},\n",
                    s.second, s.phase, s.frenzied, s.attack, s.stage, s.shieldKind, s.shieldUp,
                    static_cast<double>(s.shieldStrength), s.bossHealth, static_cast<double>(s.bossX),
                    static_cast<double>(s.bossY), s.playerHealth, static_cast<double>(s.playerX),
                    static_cast<double>(s.playerY), s.hairballs, s.hostileProjectiles, s.shotsFired, s.shotsHit,
                    static_cast<unsigned long long>(s.randomState));
    }
    std::printf("END\n");
    const Coverage& c = run.coverage;
    std::printf("coverage: attacks %d%d%d%d%d, phase two %d, frenzy %d, breaks %d, hits %d, hairballs %d, won %d\n",
                c.attackSeen[0], c.attackSeen[1], c.attackSeen[2], c.attackSeen[3], c.attackSeen[4],
                c.reachedPhaseTwo, c.frenzied, c.shieldBreaks, c.hitsTaken, c.mostHairballs, c.won);
}
