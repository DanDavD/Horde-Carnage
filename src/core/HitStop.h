#pragma once

namespace horde {

/// How deep a hit stop slows the fight.
struct HitStopStats {
    /// The fight's speed at the moment of the hit, as a share of normal speed. Not 0: a world that stops
    /// dead for a few frames looks like the game hitched, not like a heavy hit.
    float slowestScale = 0.08f;
};

/// A hit stop: when a heavy hit lands, the fight drops to a crawl and eases back to full speed, so the hit
/// reads as heavy.
///
/// It slows the fight's clock instead of holding the world still. The world keeps stepping, only less often,
/// and the drawing keeps blending between steps, so every frame moves a little and the slow-down never
/// looks like a dropped frame. It runs on real time, not on the fight's: it decides how fast the fight runs.
struct HitStop {
    float secondsLeft = 0.0f; ///< Real seconds until the fight is back to full speed.
    float seconds = 0.0f;     ///< How long the current slow-down lasts in all.
    HitStopStats stats;
};

/// Slows the fight for `seconds` of real time. A slow-down already running is only replaced by one that
/// lasts longer than what it has left, so a stronger hit takes over and a weaker one adds nothing. Adding
/// them up would let the SMG, or a crowd, keep the fight slow for as long as they keep landing.
void StartHitStop(HitStop& stop, float seconds);

/// Moves the slow-down on by `realSeconds`, the length of the frame.
void StepHitStop(HitStop& stop, float realSeconds);

/// How fast the fight runs right now, from `stats.slowestScale` at the hit to 1. It eases out along a
/// smooth curve, so the fight picks up speed gently instead of snapping back.
[[nodiscard]] float TimeScale(const HitStop& stop);

} // namespace horde
