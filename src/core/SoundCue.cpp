#include "core/SoundCue.h"

#include <algorithm>

#include "core/AttackChoice.h"
#include "core/ImpactFeel.h"
#include "core/Weapon.h"

namespace horde {

namespace {

// How far a cue's pitch and volume wander from one play to the next.
constexpr float kPitchWander = 0.06f;
constexpr float kVolumeWander = 0.1f;

// The warning each attack makes, so a player who looks away still hears what is coming.
SoundId WarningOf(AttackKind kind) {
    switch (kind) {
    case AttackKind::Charge:
        return SoundId::ChargeGrowl;
    case AttackKind::HairRing:
        return SoundId::RingSwell;
    case AttackKind::Leap:
        return SoundId::LeapGrunt;
    case AttackKind::Volley:
        return SoundId::VolleyHiss;
    case AttackKind::Summon:
        return SoundId::SummonHowl;
    }
    return SoundId::ChargeGrowl;
}

std::optional<SoundCue> BaseCue(const WorldEvent& event) {
    switch (event.kind) {
    case WorldEventKind::ShotFired:
        // The guns are told apart by their damage: the pistol hits hard and sounds it.
        return event.amount >= kPistol.damage ? SoundCue{SoundId::PistolShot, 0.7f}
                                              : SoundCue{SoundId::SmgShot, 0.5f};
    case WorldEventKind::DashStarted:
        return SoundCue{SoundId::Dash, 0.55f};
    case WorldEventKind::PlayerHurt:
        return event.amount >= kHeavyHitDamage ? SoundCue{SoundId::PlayerHeavyHurt, 1.0f}
                                               : SoundCue{SoundId::PlayerHurt, 0.85f};
    case WorldEventKind::PlayerKilled:
        return SoundCue{SoundId::PlayerDeath, 1.0f};
    // Quiet: the SMG lands ten of these a second.
    case WorldEventKind::BossHurt:
        return SoundCue{SoundId::BossFlesh, 0.4f};
    // A bounce is a ping; a hit the shield soaks up is a thud.
    case WorldEventKind::ShieldStruck:
        return event.amount == 0 ? SoundCue{SoundId::ShellPing, 0.45f} : SoundCue{SoundId::ShieldKnock, 0.45f};
    case WorldEventKind::ShieldBroken:
        return SoundCue{SoundId::ShieldShatter, 1.0f};
    case WorldEventKind::HairballKilled:
        return SoundCue{SoundId::HairballBurst, 0.7f};
    case WorldEventKind::BossLanded:
        return SoundCue{SoundId::BossLanding, 1.0f};
    case WorldEventKind::BossHitWall:
        return SoundCue{SoundId::WallCrash, 0.9f};
    case WorldEventKind::FirstPhaseEnded:
        return SoundCue{SoundId::BossCollapse, 1.0f};
    case WorldEventKind::BossRoared:
        return SoundCue{SoundId::BossRoar, 1.0f};
    case WorldEventKind::BossDefeated:
        return SoundCue{SoundId::BossDeath, 1.0f};
    case WorldEventKind::AttackStarted:
        return SoundCue{WarningOf(static_cast<AttackKind>(event.amount)), 0.8f};
    // A whole ring is louder than one hairball's single strand.
    case WorldEventKind::StrandsSpat:
        return SoundCue{SoundId::Spit, event.amount > 1 ? 0.6f : 0.35f};
    case WorldEventKind::FlaskDrunk:
        return SoundCue{SoundId::FlaskDrink, 0.7f};
    }
    return std::nullopt;
}

} // namespace

std::optional<SoundCue> CueFor(const WorldEvent& event, Random& random) {
    std::optional<SoundCue> cue = BaseCue(event);
    if (cue) {
        cue->pitch *= 1.0f + RandomRange(random, -kPitchWander, kPitchWander);
        cue->volume *= 1.0f + RandomRange(random, -kVolumeWander, kVolumeWander);
    }
    return cue;
}

SoundCue Placed(SoundCue cue, Vec2 source, Vec2 listener, float viewWidth) {
    if (viewWidth <= 0.0f) {
        return cue;
    }
    const float across = (source.x - listener.x) / (0.5f * viewWidth);
    cue.pan = std::clamp(across, -1.0f, 1.0f) * kMaxPan;
    const float offScreen = std::max(Length(source - listener) - 0.5f * viewWidth, 0.0f) / viewWidth;
    cue.volume *= 1.0f - 0.5f * std::min(offScreen, 1.0f);
    return cue;
}

} // namespace horde
