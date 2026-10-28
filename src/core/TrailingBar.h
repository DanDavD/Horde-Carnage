#pragma once

namespace horde {

/// How the pale trail behind a health bar waits and drains.
struct TrailingBarStats {
    float holdSeconds = 0.6f; ///< How long the trail waits after the last hit before it drains.
    float drainShare = 0.6f;  ///< How much of the whole bar the trail drains per second.
};

/// A health bar's trail, in the manner of Dark Souls: a hit takes the bar down at once, but a pale strip
/// stays where the bar was, waits while the hits keep coming, then drains down to the bar. The strip is how
/// much the last run of hits took, so a big hit reads as big even though the bar dropped in one frame.
///
/// It only follows a health value; it never changes one. The game keeps one for the player and one for the
/// boss, next to the HUD that draws them.
struct TrailingBar {
    float trail = 0.0f;           ///< The health the trail reaches to. Never below the health itself.
    int lastHealth = 0;           ///< The health it saw last step, to tell a hit from a heal.
    float holdSecondsLeft = 0.0f; ///< Above 0 while the trail waits for more hits.
    int tally = 0;                ///< The damage of the current run of hits, shown next to the boss's bar.
    TrailingBarStats stats;
};

/// A trail with nothing behind the bar, at `health`.
[[nodiscard]] TrailingBar TrailingBarAt(int health);

/// Follows `health` (out of `maxHealth`) for one step of `dt` seconds. A hit starts the hold again and adds
/// to the tally; a hit after the trail has caught up starts a new tally. A heal lifts the trail with the
/// bar when it passes it.
void StepTrailingBar(TrailingBar& bar, int health, int maxHealth, float dt);

/// Whether there is a trail to draw behind `health`, and so a tally to show.
[[nodiscard]] bool IsTrailing(const TrailingBar& bar, int health);

} // namespace horde
