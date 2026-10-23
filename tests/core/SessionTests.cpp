#include <cstddef>

#include <doctest/doctest.h>

#include "core/Session.h"

using horde::Arena;
using horde::Enemy;
using horde::Session;
using horde::SessionState;
using horde::Vec2;

namespace {

constexpr Arena kArena{{0.0f, 0.0f}, {2400.0f, 1600.0f}};
constexpr Vec2 kView{1280.0f, 720.0f};
constexpr float kDt = 1.0f / 60.0f;

void Step(Session& session, Vec2 move, int steps) {
    for (int i = 0; i < steps; ++i) {
        horde::StepSession(session, horde::PlayerCommands{.move = move}, kDt);
    }
}

// One hairball already touching the player, and the player one hit from death.
Session OneHitFromDeath() {
    Session session = horde::StartSession(kArena, kView);
    horde::Player& player = session.world.player;
    player.health.current = 10;
    session.world.enemies.assign(
        1, Enemy{.motion = horde::MotionAt(player.motion.position + Vec2{player.radius + 18.0f, 0.0f})});
    return session;
}

} // namespace

TEST_CASE("a new session is playing, with the player at full health") {
    const Session session = horde::StartSession(kArena, kView);
    CHECK(session.state == SessionState::Playing);
    CHECK(session.world.player.health.current == session.world.player.health.max);
    CHECK(session.world.elapsedSeconds == 0.0f);
}

TEST_CASE("while playing, a session step is a world step") {
    Session session = horde::StartSession(kArena, kView);
    horde::World expected = horde::CreateWorld(kArena, kView);
    horde::StepWorld(expected, horde::PlayerCommands{.move = {1.0f, 0.0f}}, kDt);

    Step(session, Vec2{1.0f, 0.0f}, 1);
    CHECK(session.world.player.motion.position == expected.player.motion.position);
    CHECK(session.world.elapsedSeconds == doctest::Approx(kDt));
}

TEST_CASE("the game is over in the step the player's health reaches zero") {
    Session session = OneHitFromDeath();
    Step(session, Vec2{}, 1);
    CHECK(horde::IsDead(session.world.player.health));
    CHECK(session.state == SessionState::GameOver);
}

TEST_CASE("once the game is over the world is frozen") {
    Session session = OneHitFromDeath();
    Step(session, Vec2{}, 1);
    const Vec2 player = session.world.player.motion.position;
    const Vec2 enemy = session.world.enemies[0].motion.position;
    const float survived = session.world.elapsedSeconds;

    Step(session, Vec2{1.0f, 0.0f}, 120);
    CHECK(session.world.player.motion.position == player);
    CHECK(session.world.enemies[0].motion.position == enemy);
    CHECK(session.world.elapsedSeconds == survived);
}

TEST_CASE("restart is refused while playing and right after death") {
    Session session = horde::StartSession(kArena, kView);
    CHECK_FALSE(horde::TryRestart(session));
    CHECK(session.state == SessionState::Playing);

    session = OneHitFromDeath();
    Step(session, Vec2{}, 1);
    Step(session, Vec2{}, 30); // Half a second, inside the restart delay.
    CHECK_FALSE(horde::CanRestart(session));
    CHECK_FALSE(horde::TryRestart(session));
    CHECK(session.state == SessionState::GameOver);
}

TEST_CASE("after the delay a restart starts a fresh world") {
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
    REQUIRE(session.world.enemies.size() == fresh.enemies.size());
    for (std::size_t i = 0; i < fresh.enemies.size(); ++i) {
        CHECK(session.world.enemies[i].motion.position == fresh.enemies[i].motion.position);
    }
}

TEST_CASE("standing still among the hairballs ends in a game over") {
    // Six hairballs, 10 damage per touch, one hit landing every 0.8 s: dead in well under a minute.
    Session session = horde::StartSession(kArena, kView);
    int steps = 0;
    while (session.state == SessionState::Playing && steps < 60 * 60) {
        Step(session, Vec2{}, 1);
        ++steps;
    }
    CHECK(session.state == SessionState::GameOver);
    CHECK(session.world.elapsedSeconds > 5.0f);
}
