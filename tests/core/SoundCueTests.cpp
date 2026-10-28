#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

#include <doctest/doctest.h>

#include "core/AttackChoice.h"
#include "core/ImpactFeel.h"
#include "core/SoundCue.h"
#include "core/SoundLibrary.h"
#include "core/Weapon.h"

using horde::SoundCue;
using horde::SoundId;
using horde::WorldEvent;
using horde::WorldEventKind;

namespace {

std::optional<SoundCue> Cue(WorldEventKind kind, int amount = 0) {
    horde::Random random = horde::SeedRandom(5);
    return horde::CueFor(WorldEvent{.kind = kind, .amount = amount}, random);
}

SoundId IdOf(WorldEventKind kind, int amount = 0) {
    const std::optional<SoundCue> cue = Cue(kind, amount);
    REQUIRE(cue);
    return cue->id;
}

} // namespace

TEST_CASE("every event the fight records makes a sound") {
    for (int kind = 0; kind <= static_cast<int>(WorldEventKind::FlaskDrunk); ++kind) {
        CHECK(Cue(static_cast<WorldEventKind>(kind)).has_value());
    }
}

TEST_CASE("each gun sounds like itself, and a heavy hit like a heavy hit") {
    CHECK(IdOf(WorldEventKind::ShotFired, horde::kPistol.damage) == SoundId::PistolShot);
    CHECK(IdOf(WorldEventKind::ShotFired, horde::kSmg.damage) == SoundId::SmgShot);
    CHECK(IdOf(WorldEventKind::PlayerHurt, 15) == SoundId::PlayerHurt);
    CHECK(IdOf(WorldEventKind::PlayerHurt, horde::kHeavyHitDamage) == SoundId::PlayerHeavyHurt);
}

TEST_CASE("a bullet that bounces off the shell pings, one the shield soaks up thuds") {
    CHECK(IdOf(WorldEventKind::ShieldStruck, 0) == SoundId::ShellPing);
    CHECK(IdOf(WorldEventKind::ShieldStruck, 6) == SoundId::ShieldKnock);
}

TEST_CASE("every attack warns with its own sound") {
    const std::vector<SoundId> warnings{
        IdOf(WorldEventKind::AttackStarted, static_cast<int>(horde::AttackKind::Charge)),
        IdOf(WorldEventKind::AttackStarted, static_cast<int>(horde::AttackKind::HairRing)),
        IdOf(WorldEventKind::AttackStarted, static_cast<int>(horde::AttackKind::Leap)),
        IdOf(WorldEventKind::AttackStarted, static_cast<int>(horde::AttackKind::Volley)),
        IdOf(WorldEventKind::AttackStarted, static_cast<int>(horde::AttackKind::Summon))};
    for (std::size_t i = 0; i < warnings.size(); ++i) {
        for (std::size_t j = i + 1; j < warnings.size(); ++j) {
            CHECK(warnings[i] != warnings[j]);
        }
    }
}

TEST_CASE("bullets on the boss are quieter than the hits that matter") {
    CHECK(Cue(WorldEventKind::BossHurt)->volume < Cue(WorldEventKind::ShieldBroken)->volume);
    CHECK(Cue(WorldEventKind::BossHurt)->volume < Cue(WorldEventKind::PlayerHurt, 15)->volume);
}

TEST_CASE("each play wanders a little in pitch and volume, and never far") {
    horde::Random random = horde::SeedRandom(9);
    const WorldEvent shot{.kind = WorldEventKind::ShotFired, .amount = horde::kSmg.damage};
    const SoundCue first = *horde::CueFor(shot, random);
    bool changed = false;
    for (int i = 0; i < 50; ++i) {
        const SoundCue cue = *horde::CueFor(shot, random);
        CHECK(std::abs(cue.pitch - 1.0f) <= 0.06f);
        CHECK(cue.volume > 0.0f);
        CHECK(cue.volume <= 1.1f);
        changed = changed || cue.pitch != first.pitch;
    }
    CHECK(changed);
}

TEST_CASE("every sound has a recipe that makes a short, clean sound") {
    for (std::size_t i = 0; i < horde::kSoundCount; ++i) {
        const horde::SoundRecipe& recipe = horde::RecipeFor(static_cast<SoundId>(i));
        CHECK(recipe.layerCount > 0);
        CHECK(recipe.seconds > 0.0f);
        CHECK(recipe.seconds <= 3.0f);
        const std::vector<float> samples = horde::Synthesize(recipe);
        float peak = 0.0f;
        for (const float sample : samples) {
            peak = std::max(peak, std::abs(sample));
        }
        CHECK(peak > 0.5f);
        CHECK(peak <= 0.9f);
    }
}

TEST_CASE("a sound leans to the side it comes from, and fades the further off screen it is") {
    const SoundCue cue{.volume = 1.0f};
    const horde::Vec2 listener{500.0f, 500.0f};
    constexpr float kWidth = 1280.0f;
    CHECK(horde::Placed(cue, listener, listener, kWidth).pan == doctest::Approx(0.0f));
    CHECK(horde::Placed(cue, listener, listener, kWidth).volume == doctest::Approx(1.0f));
    CHECK(horde::Placed(cue, listener + horde::Vec2{320.0f, 0.0f}, listener, kWidth).pan ==
          doctest::Approx(horde::kMaxPan * 0.5f));
    CHECK(horde::Placed(cue, listener - horde::Vec2{5000.0f, 0.0f}, listener, kWidth).pan ==
          doctest::Approx(-horde::kMaxPan));
    // On screen it keeps its volume; a whole view beyond the edge, half; never less.
    CHECK(horde::Placed(cue, listener + horde::Vec2{600.0f, 0.0f}, listener, kWidth).volume == doctest::Approx(1.0f));
    CHECK(horde::Placed(cue, listener + horde::Vec2{1920.0f, 0.0f}, listener, kWidth).volume == doctest::Approx(0.5f));
    CHECK(horde::Placed(cue, listener + horde::Vec2{9000.0f, 0.0f}, listener, kWidth).volume == doctest::Approx(0.5f));
}
