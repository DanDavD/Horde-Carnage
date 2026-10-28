#include <cstdint>

#include <doctest/doctest.h>

#include "core/Session.h"

using horde::Arena;
using horde::PlayerCommands;
using horde::Session;
using horde::SessionState;
using horde::Vec2;

namespace {

constexpr Arena kArena = horde::kBossArena;
constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

void Step(Session& session, const PlayerCommands& commands, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepSession(session, commands, kDt);
    }
}

void Step(Session& session, Vec2 move, int steps) {
    Step(session, PlayerCommands{.move = move}, steps);
}

// The boss already touching the player, and the player one touch from death.
void PutOneHitFromDeath(horde::World& world) {
    world.player.health.current = world.boss.contactDamage;
    const float contact = world.boss.radius + world.player.radius;
    world.boss.motion = horde::MotionAt(world.player.motion.position - Vec2{0.0f, contact});
}

Session OneHitFromDeath() {
    Session session = horde::StartSession(kArena, kView);
    PutOneHitFromDeath(session.world);
    return session;
}

// Dies, waits out the restart delay and retries.
void DieAndRetry(Session& session) {
    PutOneHitFromDeath(session.world);
    Step(session, Vec2{}, 1);
    REQUIRE(session.state == SessionState::Dead);
    Step(session, Vec2{}, 61);
    REQUIRE(horde::TryRestart(session));
}

} // namespace

TEST_CASE("a new session is the first attempt, playing, with the player at full health") {
    const Session session = horde::StartSession(kArena, kView);
    CHECK(session.state == SessionState::Playing);
    CHECK(session.attempt == 1);
    CHECK(session.world.player.health.current == session.world.player.health.max);
    CHECK(session.world.elapsedSeconds == 0.0f);
}

TEST_CASE("while playing, a session step is a world step") {
    Session session = horde::StartSession(kArena, kView);
    horde::World expected = horde::CreateWorld(kArena, kView);
    horde::StepWorld(expected, PlayerCommands{.move = {1.0f, 0.0f}}, kDt);

    Step(session, Vec2{1.0f, 0.0f}, 1);
    CHECK(session.world.player.motion.position == expected.player.motion.position);
    CHECK(session.world.elapsedSeconds == doctest::Approx(kDt));
}

TEST_CASE("the player is dead in the step their health reaches zero") {
    Session session = OneHitFromDeath();
    Step(session, Vec2{}, 1);
    CHECK(horde::IsDead(session.world.player.health));
    CHECK(session.state == SessionState::Dead);
}

TEST_CASE("a dead player's world remembers what killed them") {
    Session session = OneHitFromDeath();
    Step(session, Vec2{}, 1);
    REQUIRE(session.state == SessionState::Dead);
    CHECK(session.world.lastHitBy == horde::DamageSource::BossTouch);
}

TEST_CASE("once the player is dead the world is frozen") {
    Session session = OneHitFromDeath();
    Step(session, Vec2{}, 1);
    const Vec2 player = session.world.player.motion.position;
    const Vec2 boss = session.world.boss.motion.position;
    const float survived = session.world.elapsedSeconds;

    Step(session, Vec2{1.0f, 0.0f}, 120);
    CHECK(session.world.player.motion.position == player);
    CHECK(session.world.boss.motion.position == boss);
    CHECK(session.world.elapsedSeconds == survived);
}

TEST_CASE("the fight is won in the step the last bullet of the second phase lands") {
    Session session = horde::StartSession(kArena, kView);
    horde::Boss& boss = session.world.boss;
    boss.phase = horde::BossPhase::Two;
    boss.health.current = horde::kPistol.damage;
    horde::BreakShield(boss.shield);

    // The player starts aiming at the boss, so one shot is enough.
    Step(session, PlayerCommands{.firePressed = true}, 1);
    int steps = 1;
    while (session.state == SessionState::Playing && steps < 60) {
        Step(session, Vec2{}, 1);
        ++steps;
    }
    CHECK(session.state == SessionState::Victory);
    CHECK(horde::IsDead(boss.health));
    CHECK(session.stateSeconds == 0.0f);
}

TEST_CASE("once the fight is won the world is frozen") {
    Session session = horde::StartSession(kArena, kView);
    Step(session, Vec2{}, 30);
    horde::DefeatBoss(session.world);
    Step(session, Vec2{}, 1);
    REQUIRE(session.state == SessionState::Victory);
    const Vec2 player = session.world.player.motion.position;
    const float fightLength = session.world.elapsedSeconds;

    Step(session, PlayerCommands{.move = {1.0f, 0.0f}, .firePressed = true}, 120);
    CHECK(session.world.player.motion.position == player);
    CHECK(session.world.elapsedSeconds == fightLength);
    CHECK(session.world.shotsFired == 0);
}

TEST_CASE("when the boss and the player fall in the same step, the fight is won") {
    Session session = horde::StartSession(kArena, kView);
    horde::World& world = session.world;
    horde::DefeatBoss(world);
    // A hairball against the player, one touch from death.
    const horde::Enemy hairball{};
    world.player.health.current = hairball.contactDamage;
    const float contact = world.player.radius + hairball.radius;
    world.enemies.Spawn(horde::Enemy{.motion = horde::MotionAt(world.player.motion.position + Vec2{contact, 0.0f})});

    Step(session, Vec2{}, 1);
    REQUIRE(horde::IsDead(world.player.health));
    CHECK(session.state == SessionState::Victory);
}

TEST_CASE("restart is refused while playing and right after the fight ends") {
    Session session = horde::StartSession(kArena, kView);
    CHECK_FALSE(horde::TryRestart(session));
    CHECK(session.state == SessionState::Playing);

    session = OneHitFromDeath();
    Step(session, Vec2{}, 1);
    Step(session, Vec2{}, 30); // Half a second, inside the restart delay.
    CHECK_FALSE(horde::CanRestart(session));
    CHECK_FALSE(horde::TryRestart(session));
    CHECK(session.state == SessionState::Dead);

    session = horde::StartSession(kArena, kView);
    horde::DefeatBoss(session.world);
    Step(session, Vec2{}, 1);
    Step(session, Vec2{}, 30);
    CHECK_FALSE(horde::TryRestart(session));
    CHECK(session.state == SessionState::Victory);
}

TEST_CASE("a death shows its screen at once, a victory only after the boss has died on screen") {
    Session dead = OneHitFromDeath();
    Step(dead, Vec2{}, 1);
    CHECK(horde::EndScreenSeconds(dead) == doctest::Approx(0.0f));
    Step(dead, Vec2{}, 61);
    CHECK(horde::CanRestart(dead));

    Session won = horde::StartSession(kArena, kView);
    CHECK(horde::EndScreenSeconds(won) == 0.0f);
    horde::DefeatBoss(won.world);
    Step(won, Vec2{}, 1);
    REQUIRE(won.state == SessionState::Victory);
    CHECK(horde::EndScreenSeconds(won) < 0.0f);
    // Past the restart delay, but the screen has not shown yet: the boss is still dying.
    Step(won, Vec2{}, 61);
    CHECK(horde::EndScreenSeconds(won) < 0.0f);
    CHECK_FALSE(horde::CanRestart(won));
    // The reveal and then the delay.
    Step(won, Vec2{}, 90);
    CHECK(horde::EndScreenSeconds(won) == doctest::Approx(1.0f).epsilon(0.02));
    CHECK(horde::CanRestart(won));
}

TEST_CASE("after the delay a retry starts a fresh world") {
    Session session = OneHitFromDeath();
    Step(session, Vec2{}, 1);
    Step(session, Vec2{}, 61); // Just over the one second delay.
    REQUIRE(horde::CanRestart(session));
    CHECK(horde::TryRestart(session));

    const horde::World fresh = horde::CreateWorld(kArena, kView);
    CHECK(session.state == SessionState::Playing);
    CHECK(session.stateSeconds == 0.0f);
    CHECK(session.world.player.health.current == session.world.player.health.max);
    CHECK(session.world.elapsedSeconds == 0.0f);
    CHECK(session.world.player.motion.position == fresh.player.motion.position);
    CHECK(session.world.boss.motion.position == fresh.boss.motion.position);
    CHECK(session.world.boss.health.current == fresh.boss.health.max);
    CHECK(session.world.enemies.Empty());
    CHECK(session.world.bullets.Empty());
}

TEST_CASE("every retry after a death is the next attempt") {
    Session session = horde::StartSession(kArena, kView);
    DieAndRetry(session);
    CHECK(session.attempt == 2);
    DieAndRetry(session);
    CHECK(session.attempt == 3);
}

TEST_CASE("a new fight after a victory starts the attempts over") {
    Session session = horde::StartSession(kArena, kView);
    DieAndRetry(session);
    DieAndRetry(session);
    REQUIRE(session.attempt == 3);

    horde::DefeatBoss(session.world);
    Step(session, Vec2{}, 1);
    REQUIRE(session.state == SessionState::Victory);
    CHECK(session.attempt == 3); // The victory screen still shows which attempt won.
    Step(session, Vec2{}, 151); // The boss's death, then the restart delay.
    REQUIRE(horde::TryRestart(session));
    CHECK(session.attempt == 1);
    CHECK(session.state == SessionState::Playing);
    CHECK(session.world.boss.health.current == session.world.boss.health.max);
}

TEST_CASE("every attempt gets its own seed, and the session's seed decides them all") {
    Session a = horde::StartSession(kArena, kView, 5);
    Session b = horde::StartSession(kArena, kView, 5);
    CHECK(a.world.seed == b.world.seed);
    CHECK(horde::StartSession(kArena, kView, 6).world.seed != a.world.seed);

    const std::uint64_t first = a.world.seed;
    DieAndRetry(a);
    DieAndRetry(b);
    CHECK(a.world.seed != first);
    CHECK(a.world.seed == b.world.seed);
    // The world's generator starts from its seed.
    horde::Random fresh = horde::SeedRandom(a.world.seed);
    CHECK(horde::NextU32(a.world.random) == horde::NextU32(fresh));
}

TEST_CASE("standing still in front of the boss ends in death, whatever the seed") {
    // The fight opens with a charge after 2 s, and from there the boss's picks depend on the seed. Over these
    // 20 seeds a player who never moves dies between 6.3 and 18.8 s, most often to the boss's touch once a
    // charge has left it standing next to them.
    for (std::uint64_t seed = 1; seed <= 20; ++seed) {
        Session session = horde::StartSession(kArena, kView, seed);
        int steps = 0;
        while (session.state == SessionState::Playing && steps < 60 * 60) {
            Step(session, Vec2{}, 1);
            ++steps;
        }
        CHECK(session.state == SessionState::Dead);
        CHECK(session.world.elapsedSeconds < 20.0f);
        CHECK(session.world.lastHitBy.has_value());
    }
}

TEST_CASE("the title screen waits over a fresh world that does not move until the fight begins") {
    Session session = horde::StartSession(kArena, kView);
    horde::ReturnToTitle(session);
    CHECK(session.state == SessionState::Title);
    const Vec2 player = session.world.player.motion.position;
    Step(session, Vec2{1.0f, 0.0f}, 60);
    CHECK(session.world.player.motion.position == player);
    CHECK(session.world.elapsedSeconds == 0.0f);

    CHECK(horde::BeginFight(session));
    CHECK(session.state == SessionState::Playing);
    Step(session, Vec2{1.0f, 0.0f}, 10);
    CHECK(session.world.player.motion.position.x > player.x);
    CHECK_FALSE(horde::BeginFight(session)); // Only from the title.
}

TEST_CASE("a paused fight holds still, and resumes from where it stopped") {
    Session session = horde::StartSession(kArena, kView);
    Step(session, Vec2{1.0f, 0.0f}, 10);
    REQUIRE(horde::PauseFight(session));
    CHECK(session.state == SessionState::Paused);
    const Vec2 player = session.world.player.motion.position;
    const float clock = session.world.elapsedSeconds;
    Step(session, PlayerCommands{.move = Vec2{1.0f, 0.0f}, .firePressed = true}, 120);
    CHECK(session.world.player.motion.position == player);
    CHECK(session.world.elapsedSeconds == clock);
    CHECK(session.world.bullets.Empty());
    CHECK_FALSE(horde::CanRestart(session));
    CHECK_FALSE(horde::TryRestart(session));

    CHECK(horde::ResumeFight(session));
    CHECK(session.state == SessionState::Playing);
    Step(session, Vec2{1.0f, 0.0f}, 1);
    CHECK(session.world.elapsedSeconds > clock);
    CHECK_FALSE(horde::ResumeFight(session)); // Only a paused fight.
}

TEST_CASE("only a fight in progress can pause") {
    Session session = OneHitFromDeath();
    Step(session, Vec2{}, 1);
    REQUIRE(session.state == SessionState::Dead);
    CHECK_FALSE(horde::PauseFight(session));
    horde::ReturnToTitle(session);
    CHECK_FALSE(horde::PauseFight(session));
}

TEST_CASE("going back to the title gives a fresh world and starts the attempts over") {
    Session session = horde::StartSession(kArena, kView);
    DieAndRetry(session);
    DieAndRetry(session);
    REQUIRE(session.attempt == 3);
    const std::uint64_t seed = session.world.seed;
    Step(session, Vec2{1.0f, 0.0f}, 30);
    REQUIRE(horde::PauseFight(session));

    horde::ReturnToTitle(session);
    const horde::World fresh = horde::CreateWorld(kArena, kView);
    CHECK(session.state == SessionState::Title);
    CHECK(session.attempt == 1);
    CHECK(session.world.seed != seed);
    CHECK(session.world.elapsedSeconds == 0.0f);
    CHECK(session.world.player.motion.position == fresh.player.motion.position);
}
