#pragma once

namespace horde {

/// What hurt the player. The world remembers the last hit that landed, so the death screen can say what
/// killed them: in a fight learned by dying, knowing which attack did it is the lesson.
enum class DamageSource {
    BossTouch,   ///< Walking into the boss, or the boss walking into the player.
    Charge,      ///< Caught by the boss's run.
    HairRing,    ///< One strand of a Hair Ring.
    Volley,      ///< One strand of an Aimed Volley.
    LeapImpact,  ///< Under the boss when it came down.
    Shockwave,   ///< The ring that rolls out from a landing, or from the roar that opens the second phase.
    Hairball,    ///< One of the small ones.
    Frenzy,      ///< The strands its attacks throw off once it is in a frenzy.
};

} // namespace horde
