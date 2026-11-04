#pragma once

namespace horde {

/// Every attack runs through the same three stages, so the player learns one rhythm: a warning that shows
/// what is coming and where, the attack itself, and a recovery that is the opening to punish it.
enum class AttackStage {
    Warning,
    Active,
    Recovery,
};

/// Where an attack is in its three stages, and how long it has been there.
///
/// Every attack of every boss keeps time with one of these instead of its own counter, so they all follow the
/// same rules: the clock is advanced once at the top of the attack's step, before anything reads it, and a
/// new stage starts from zero. A stage of a whole number of steps then ends on its last step, never one late.
struct AttackClock {
    AttackStage stage = AttackStage::Warning;
    float seconds = 0.0f; ///< Time spent in the current stage.
};

/// Advances the clock by one step of `dt` seconds.
void TickClock(AttackClock& clock, float dt);

/// Moves to `stage` and starts it from zero.
void EnterStage(AttackClock& clock, AttackStage stage);

/// Whether the current stage has lasted at least `length` seconds.
[[nodiscard]] bool HasLasted(const AttackClock& clock, float length);

/// Whether beat `index` (counting from 0) of a series `interval` seconds apart is due. The first beat falls at
/// the start of the stage. Several attacks fire on beats like these: the waves of a ring, the bursts of a
/// volley, the strands thrown during a charge.
[[nodiscard]] bool IsBeatDue(const AttackClock& clock, int index, float interval);

/// Whether the last of `count` beats `interval` seconds apart has passed, and `tail` seconds after it.
[[nodiscard]] bool HasLastedPastBeats(const AttackClock& clock, int count, float interval, float tail);

} // namespace horde
