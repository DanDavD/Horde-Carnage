#include "core/ImpactFeel.h"

namespace horde {

ImpactFeel FeelOf(const WorldEvent& event) {
    switch (event.kind) {
    // Many times a second, or the player's own dash: the particles and sounds carry these.
    case WorldEventKind::ShotFired:
    case WorldEventKind::DashStarted:
    case WorldEventKind::BossHurt:
    case WorldEventKind::ShieldStruck:
        return {};
    // The player's own hits. Slowing on them is what makes a hit read as a hit and not as a number going
    // down, and it gives a beat to see what landed.
    case WorldEventKind::PlayerHurt:
        return event.amount >= kHeavyHitDamage ? ImpactFeel{.slowSeconds = 0.3f, .trauma = 0.65f}
                                               : ImpactFeel{.slowSeconds = 0.18f, .trauma = 0.45f};
    case WorldEventKind::PlayerKilled:
        return {.slowSeconds = 0.6f, .trauma = 0.8f};
    // The player's wins: a break or a kill is a reward, and a short slow-down lets it land.
    case WorldEventKind::ShieldBroken:
        return {.slowSeconds = 0.25f, .trauma = 0.55f};
    case WorldEventKind::HairballKilled:
        return {.slowSeconds = 0.1f, .trauma = 0.2f};
    // The boss's weight. These shake without slowing, because they happen whether or not they hit, and the
    // player has to keep moving through them. The crash into a wall gets a touch, as it opens a window.
    case WorldEventKind::BossLanded:
        return {.trauma = 0.5f};
    case WorldEventKind::BossHitWall:
        return {.slowSeconds = 0.12f, .trauma = 0.55f};
    case WorldEventKind::BossRoared:
        return {.trauma = 0.7f};
    // The two big moments of the fight.
    case WorldEventKind::FirstPhaseEnded:
        return {.slowSeconds = 0.45f, .trauma = 0.6f};
    case WorldEventKind::BossDefeated:
        return {.slowSeconds = 0.8f, .trauma = 0.9f};
    }
    return {};
}

} // namespace horde
