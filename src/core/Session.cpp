#include "core/Session.h"

#include "core/Health.h"

namespace horde {

namespace {

constexpr std::uint64_t kSeedStream = 7;

void EnterState(Session& session, SessionState state) {
    session.state = state;
    session.stateSeconds = 0.0f;
}

} // namespace

Session StartSession(const Arena& arena, Vec2 viewSize, std::uint64_t seed) {
    // Its own stream, so the world seeds it hands out are not the numbers the first world draws.
    Random seeds = SeedRandom(seed, kSeedStream);
    const std::uint32_t first = NextU32(seeds);
    return Session{.world = CreateWorld(arena, viewSize, first), .seeds = seeds};
}

void StepSession(Session& session, const PlayerCommands& commands, float dt) {
    session.stateSeconds += dt;

    switch (session.state) {
    case SessionState::Playing:
        StepWorld(session.world, commands, dt);
        // The boss is checked first. A trade, where the last bullet lands in the step the player takes a
        // killing blow, counts as a win: the player did beat the boss.
        if (IsBossDefeated(session.world)) {
            EnterState(session, SessionState::Victory);
        } else if (IsDead(session.world.player.health)) {
            EnterState(session, SessionState::Dead);
        }
        break;
    case SessionState::Dead:
    case SessionState::Victory:
        // The world stays frozen on the moment the fight ended, so the player can see what got them, or
        // the boss lying on the floor. It also stops the fight's clock for the end screen.
    case SessionState::Title:
    case SessionState::Paused:
        // Behind a menu the world does not move, and the fight's clock stands still.
        break;
    }
}

float EndScreenSeconds(const Session& session) {
    switch (session.state) {
    case SessionState::Title:
    case SessionState::Playing:
    case SessionState::Paused:
        return 0.0f;
    case SessionState::Dead:
        return session.stateSeconds;
    case SessionState::Victory:
        return session.stateSeconds - kVictoryRevealSeconds;
    }
    return 0.0f;
}

bool CanRestart(const Session& session) {
    const bool over = session.state == SessionState::Dead || session.state == SessionState::Victory;
    return over && EndScreenSeconds(session) >= kRestartDelaySeconds;
}

void ReturnToTitle(Session& session) {
    session.world = CreateWorld(session.world.arena, session.world.camera.viewSize, NextU32(session.seeds));
    session.attempt = 1;
    EnterState(session, SessionState::Title);
}

bool BeginFight(Session& session) {
    if (session.state != SessionState::Title) {
        return false;
    }
    EnterState(session, SessionState::Playing);
    return true;
}

bool PauseFight(Session& session) {
    if (session.state != SessionState::Playing) {
        return false;
    }
    EnterState(session, SessionState::Paused);
    return true;
}

bool ResumeFight(Session& session) {
    if (session.state != SessionState::Paused) {
        return false;
    }
    EnterState(session, SessionState::Playing);
    return true;
}

bool TryRestart(Session& session) {
    if (!CanRestart(session)) {
        return false;
    }
    // A death is one more try at the same boss. A win closes that run of tries, so the next fight is a new
    // challenge and starts counting from 1.
    session.attempt = session.state == SessionState::Victory ? 1 : session.attempt + 1;
    // A brand new World instead of resetting fields one by one, so a field added later cannot be
    // forgotten in the reset and leak from one run into the next.
    session.world = CreateWorld(session.world.arena, session.world.camera.viewSize, NextU32(session.seeds));
    EnterState(session, SessionState::Playing);
    return true;
}

} // namespace horde
