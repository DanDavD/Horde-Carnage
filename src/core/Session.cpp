#include "core/Session.h"

#include "core/Health.h"

namespace horde {

namespace {

void EnterState(Session& session, SessionState state) {
    session.state = state;
    session.stateSeconds = 0.0f;
}

} // namespace

Session StartSession(const Arena& arena, Vec2 viewSize) {
    return Session{.world = CreateWorld(arena, viewSize)};
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
        break;
    }
}

bool CanRestart(const Session& session) {
    return session.state != SessionState::Playing && session.stateSeconds >= kRestartDelaySeconds;
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
    session.world = CreateWorld(session.world.arena, session.world.camera.viewSize);
    EnterState(session, SessionState::Playing);
    return true;
}

} // namespace horde
