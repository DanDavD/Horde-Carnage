#pragma once

#include <cstdint>

#include "core/Arena.h"
#include "core/Random.h"
#include "core/Vec2.h"
#include "core/World.h"

namespace horde {

/// Where the game is in a run. Day 11 adds the main menu and pause as more states.
enum class SessionState {
    Playing, ///< The world steps every fixed step.
    Dead,    ///< The player died. The world is frozen until a retry.
    Victory, ///< The boss is defeated. The world is frozen until the player fights again.
};

/// After the end screen starts to show, restart input is ignored for this long. Someone mashing buttons in
/// a fight would otherwise skip the end screen before seeing it.
inline constexpr float kRestartDelaySeconds = 1.0f;

/// How long a won fight shows the boss dying before the VICTORY screen starts to fade in: the death is the
/// reward, so nothing covers it. A death shows its screen at once, because the player wants to try again.
inline constexpr float kVictoryRevealSeconds = 1.5f;

/// One run of the game: the world, the small state machine around it, and the attempt count.
///
/// Transitions: Playing goes to Victory when the boss is defeated, or to Dead when the player's health
/// reaches 0. Both go back to Playing through `TryRestart`, with a brand new World, once the end screen has
/// shown for `kRestartDelaySeconds`.
struct Session {
    World world;
    SessionState state = SessionState::Playing;
    float stateSeconds = 0.0f; ///< Simulated time spent in the current state.
    /// Which try at the boss this is, from 1. It lives here and not in the World, so a restart keeps it.
    int attempt = 1;
    /// Hands out the seed of every new World, so each attempt plays out differently, and a whole run of
    /// attempts follows from the session's one seed.
    Random seeds;
};

/// A session that starts playing straight away, in a fresh world, on the first attempt. The game passes
/// a seed from the system; tests that do not depend on chance leave the default.
[[nodiscard]] Session StartSession(const Arena& arena, Vec2 viewSize, std::uint64_t seed = 0);

/// Advances the session by one fixed step: steps the world while playing, then ends the fight if the boss
/// or the player has fallen. When both fall in the same step, the fight is won. Once the fight is over,
/// only the state clock runs, and the commands are ignored.
void StepSession(Session& session, const PlayerCommands& commands, float dt);

/// How long the end screen has been showing: the time in `Dead`, or the time in `Victory` after the boss's
/// death. Below 0 while the boss is still dying, and 0 while playing.
[[nodiscard]] float EndScreenSeconds(const Session& session);

/// Whether `TryRestart` would restart now: the fight is over and the end screen has shown for the restart
/// delay.
[[nodiscard]] bool CanRestart(const Session& session);

/// Starts a new fight in a fresh world, with the next seed, if `CanRestart` allows it. A retry after a
/// death is the next attempt; a new fight after a victory starts the count again from 1.
///
/// @return Whether the session restarted.
bool TryRestart(Session& session);

} // namespace horde
