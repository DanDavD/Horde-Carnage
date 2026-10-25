#pragma once

namespace horde {

/// What hurt the player. The world remembers the last hit that landed, so the death screen can say what
/// killed them: in a fight learned by dying, knowing which attack did it is the lesson.
enum class DamageSource {
    BossTouch,   ///< Walking into the boss, or the boss walking into the player.
    Charge,      ///< Caught by the boss's run.
    HairRing,    ///< One strand of a Hair Ring.
    LeapImpact,  ///< Under the boss when it came down.
    Shockwave,   ///< The ring that rolls out from a landing.
    Hairball,    ///< One of the small ones.
};

} // namespace horde
