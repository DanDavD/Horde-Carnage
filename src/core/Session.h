#pragma once

#include "core/Arena.h"
#include "core/Vec2.h"
#include "core/World.h"

namespace horde {

/// Where the game is in a run. Day 12 adds the main menu and pause as more states.
enum class SessionState {
    Playing,  ///< The world steps every fixed step.
    GameOver, ///< The player died. The world is frozen until a restart.
};

/// After the player dies, restart input is ignored for this long. Someone mashing buttons in a fight
/// would otherwise skip the game over screen before seeing it.
inline constexpr float kRestartDelaySeconds = 1.0f;

/// One run of the game: the world, and the small state machine around it.
///
/// Transitions: Playing goes to GameOver when the player's health reaches 0. GameOver goes back to
/// Playing through `TryRestart`, with a brand new World.
struct Session {
    World world;
    SessionState state = SessionState::Playing;
    float stateSeconds = 0.0f; ///< Simulated time spent in the current state.
};

/// A session that starts playing straight away, in a fresh world.
[[nodiscard]] Session StartSession(const Arena& arena, Vec2 viewSize);

/// Advances the session by one fixed step: steps the world while playing, and moves to GameOver when
/// the player dies. During GameOver only the state clock runs, and the commands are ignored.
void StepSession(Session& session, const PlayerCommands& commands, float dt);

/// Whether `TryRestart` would restart now: the game is over and the restart delay has passed.
[[nodiscard]] bool CanRestart(const Session& session);

/// Starts a new run in a fresh world if `CanRestart` allows it.
///
/// @return Whether the session restarted.
bool TryRestart(Session& session);

} // namespace horde
