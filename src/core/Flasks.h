#pragma once

namespace horde {

/// How healing works: a few charges per attempt, and a drink that takes long enough to be a decision.
/// Like the Estus Flask in Dark Souls, healing in the wrong moment is punished: the player is slow, cannot
/// dash or shoot, and a hit wastes the flask.
struct FlaskStats {
    int maxCharges = 3;
    float drinkSeconds = 1.0f; ///< From pressing the button to getting the health.
    int heal = 40;             ///< Hit points restored when a drink finishes.
    float moveSpeedFactor = 0.35f; ///< Fraction of top speed while drinking.
};

/// The flask component: charges left and the drink in progress.
struct Flasks {
    FlaskStats stats;
    int charges = FlaskStats{}.maxCharges;
    float drinkingSeconds = 0.0f; ///< Time left in the current drink. Above 0 means drinking.
};

[[nodiscard]] constexpr bool IsDrinking(const Flasks& flasks) {
    return flasks.drinkingSeconds > 0.0f;
}

/// Starts a drink if there is a charge left and no drink in progress. The charge is spent at once, so an
/// interrupted drink is a lost flask.
///
/// @return Whether a drink started.
bool TryStartDrinking(Flasks& flasks);

/// Advances the drink in progress by `dt` seconds.
///
/// @return The hit points to restore if the drink finished in this step, 0 otherwise.
[[nodiscard]] int StepFlasks(Flasks& flasks, float dt);

/// Stops the drink in progress without healing. The charge stays spent.
void InterruptDrinking(Flasks& flasks);

} // namespace horde
