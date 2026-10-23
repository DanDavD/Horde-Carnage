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
        if (IsDead(session.world.player.health)) {
            EnterState(session, SessionState::GameOver);
        }
        break;
    case SessionState::GameOver:
        // The world stays frozen on the moment of death, so the player can see what got them.
        break;
    }
}

bool CanRestart(const Session& session) {
    return session.state == SessionState::GameOver && session.stateSeconds >= kRestartDelaySeconds;
}

bool TryRestart(Session& session) {
    if (!CanRestart(session)) {
        return false;
    }
    // A brand new World instead of resetting fields one by one, so a field added later cannot be
    // forgotten in the reset and leak from one run into the next.
    session.world = CreateWorld(session.world.arena, session.world.camera.viewSize);
    EnterState(session, SessionState::Playing);
    return true;
}

} // namespace horde
