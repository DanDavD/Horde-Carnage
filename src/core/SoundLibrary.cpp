#include "core/SoundLibrary.h"

#include <array>
#include <cstddef>
#include <initializer_list>

namespace horde {

namespace {

using enum Waveform;

SoundRecipe Recipe(float seconds, float drive, std::initializer_list<SynthLayer> layers, std::uint64_t seed = 1) {
    SoundRecipe recipe{.seconds = seconds, .drive = drive, .seed = seed};
    for (const SynthLayer& layer : layers) {
        if (recipe.layerCount < kMaxSynthLayers) {
            recipe.layers[static_cast<std::size_t>(recipe.layerCount++)] = layer;
        }
    }
    return recipe;
}

// Most sounds are a few of the same building blocks: a burst of filtered noise for the crack or the hiss, a
// sine that drops in pitch for the weight of a body, and a saw with a slow waver for a voice.
std::array<SoundRecipe, kSoundCount> MakeRecipes() {
    std::array<SoundRecipe, kSoundCount> r{};
    const auto set = [&r](SoundId id, SoundRecipe recipe) { r[static_cast<std::size_t>(id)] = recipe; };

    // A sharp crack that darkens fast, a thump under it and a click of the hammer. The crack carries the shot:
// laptop speakers cannot play the thump at all.
    set(SoundId::PistolShot,
        Recipe(0.35f, 2.0f,
               {{.waveform = Noise, .gain = 1.8f, .attack = 0.001f, .decay = 0.06f, .sweepSeconds = 0.08f,
                 .lowPassFromHz = 7000.0f, .lowPassToHz = 1100.0f},
                {.waveform = Sine, .fromHz = 170.0f, .toHz = 50.0f, .gain = 0.7f, .decay = 0.08f,
                 .sweepSeconds = 0.1f},
                {.waveform = Square, .fromHz = 1400.0f, .toHz = 500.0f, .gain = 0.25f, .attack = 0.0005f,
                 .decay = 0.01f}}));
    // Lighter and shorter, as it fires ten times a second.
    set(SoundId::SmgShot,
        Recipe(0.18f, 1.6f,
               {{.waveform = Noise, .gain = 1.6f, .attack = 0.001f, .decay = 0.035f, .sweepSeconds = 0.05f,
                 .lowPassFromHz = 8000.0f, .lowPassToHz = 2400.0f},
                {.waveform = Sine, .fromHz = 240.0f, .toHz = 90.0f, .gain = 0.55f, .decay = 0.04f,
                 .sweepSeconds = 0.05f}},
               2));
    // Metal ringing: two sines that are not in tune with each other, over a tick.
    set(SoundId::ShellPing,
        Recipe(0.45f, 1.0f,
               {{.waveform = Sine, .fromHz = 1900.0f, .toHz = 1750.0f, .gain = 0.8f, .decay = 0.14f},
                {.waveform = Sine, .fromHz = 2870.0f, .toHz = 2700.0f, .gain = 0.45f, .decay = 0.09f},
                {.waveform = Noise, .gain = 0.6f, .attack = 0.0005f, .decay = 0.006f, .lowPassFromHz = 9000.0f,
                 .lowPassToHz = 9000.0f}},
               3));
    // A dull, soft thud with a little fizz, like a bullet into a sack.
    set(SoundId::ShieldKnock,
        Recipe(0.25f, 1.4f,
               {{.waveform = Sine, .fromHz = 320.0f, .toHz = 150.0f, .gain = 0.9f, .decay = 0.06f},
                {.waveform = Noise, .gain = 0.5f, .decay = 0.04f, .lowPassFromHz = 1800.0f,
                 .lowPassToHz = 600.0f}},
               4));
    // Glass: a long bright noise and high, inharmonic rings that die at different rates.
    set(SoundId::ShieldShatter,
        Recipe(1.0f, 1.2f,
               {{.waveform = Noise, .gain = 0.9f, .attack = 0.001f, .decay = 0.22f, .lowPassFromHz = 9500.0f,
                 .lowPassToHz = 2500.0f},
                {.waveform = Sine, .fromHz = 2350.0f, .toHz = 2200.0f, .gain = 0.35f, .decay = 0.3f},
                {.waveform = Sine, .fromHz = 3510.0f, .toHz = 3300.0f, .gain = 0.25f, .decay = 0.18f},
                {.waveform = Sine, .fromHz = 160.0f, .toHz = 60.0f, .gain = 0.7f, .decay = 0.12f}},
               5));
    // Wet and low: dark noise over a short drop.
    set(SoundId::BossFlesh,
        Recipe(0.22f, 1.5f,
               {{.waveform = Noise, .gain = 1.0f, .decay = 0.045f, .lowPassFromHz = 1100.0f, .lowPassToHz = 300.0f},
                {.waveform = Sine, .fromHz = 130.0f, .toHz = 70.0f, .gain = 0.7f, .decay = 0.05f}},
               6));
    // A pop and a squelch.
    set(SoundId::HairballBurst,
        Recipe(0.45f, 1.8f,
               {{.waveform = Noise, .gain = 1.0f, .decay = 0.09f, .lowPassFromHz = 2600.0f, .lowPassToHz = 500.0f},
                {.waveform = Sine, .fromHz = 260.0f, .toHz = 55.0f, .gain = 0.8f, .decay = 0.1f},
                {.waveform = Saw, .fromHz = 140.0f, .toHz = 60.0f, .gain = 0.3f, .decay = 0.08f,
                 .lowPassFromHz = 700.0f, .lowPassToHz = 300.0f}},
               7));
    // Air rushing past: noise whose filter opens and closes again as it fades.
    set(SoundId::Dash,
        Recipe(0.32f, 1.0f,
               {{.waveform = Noise, .gain = 1.0f, .attack = 0.04f, .decay = 0.09f, .sweepSeconds = 0.18f,
                 .lowPassFromHz = 500.0f, .lowPassToHz = 3200.0f}},
               8));
    // A rising glassy chime: the one gentle sound in the crypt.
    set(SoundId::FlaskDrink,
        Recipe(0.7f, 1.0f,
               {{.waveform = Sine, .fromHz = 520.0f, .toHz = 880.0f, .gain = 0.7f, .attack = 0.02f, .decay = 0.2f,
                 .sweepSeconds = 0.15f},
                {.waveform = Sine, .fromHz = 1320.0f, .toHz = 1320.0f, .gain = 0.35f, .delay = 0.08f,
                 .attack = 0.01f, .decay = 0.25f},
                {.waveform = Triangle, .fromHz = 1760.0f, .toHz = 1760.0f, .gain = 0.2f, .delay = 0.15f,
                 .attack = 0.01f, .decay = 0.2f}},
               9));
    // A blow to the body: a crunchy thud.
    set(SoundId::PlayerHurt,
        Recipe(0.4f, 2.5f,
               {{.waveform = Sine, .fromHz = 190.0f, .toHz = 85.0f, .gain = 1.0f, .decay = 0.1f},
                {.waveform = Noise, .gain = 0.6f, .decay = 0.04f, .lowPassFromHz = 1400.0f, .lowPassToHz = 500.0f},
                {.waveform = Square, .fromHz = 115.0f, .toHz = 70.0f, .gain = 0.25f, .decay = 0.08f}},
               10));
    set(SoundId::PlayerHeavyHurt,
        Recipe(0.6f, 3.0f,
               {{.waveform = Sine, .fromHz = 150.0f, .toHz = 45.0f, .gain = 1.0f, .decay = 0.18f},
                {.waveform = Noise, .gain = 0.8f, .decay = 0.07f, .lowPassFromHz = 2200.0f, .lowPassToHz = 300.0f},
                {.waveform = Square, .fromHz = 90.0f, .toHz = 50.0f, .gain = 0.35f, .decay = 0.14f}},
               11));
    // A long fall: everything drops and dies out.
    set(SoundId::PlayerDeath,
        Recipe(1.5f, 2.2f,
               {{.waveform = Sine, .fromHz = 140.0f, .toHz = 38.0f, .gain = 1.0f, .decay = 0.45f},
                {.waveform = Noise, .gain = 0.7f, .decay = 0.35f, .lowPassFromHz = 900.0f, .lowPassToHz = 150.0f},
                {.waveform = Saw, .fromHz = 75.0f, .toHz = 35.0f, .gain = 0.35f, .decay = 0.6f,
                 .lowPassFromHz = 500.0f, .lowPassToHz = 120.0f}},
               12));
    // A boom you feel more than hear.
    set(SoundId::BossLanding,
        Recipe(1.3f, 2.6f,
               {{.waveform = Sine, .fromHz = 75.0f, .toHz = 30.0f, .gain = 1.0f, .decay = 0.4f},
                {.waveform = Noise, .gain = 0.8f, .decay = 0.28f, .lowPassFromHz = 700.0f, .lowPassToHz = 90.0f},
                {.waveform = Sine, .fromHz = 48.0f, .toHz = 40.0f, .gain = 0.6f, .delay = 0.03f, .decay = 0.6f}},
               13));
    // Stone struck: a crack and a deep knock.
    set(SoundId::WallCrash,
        Recipe(0.9f, 2.2f,
               {{.waveform = Noise, .gain = 1.0f, .decay = 0.14f, .lowPassFromHz = 3200.0f, .lowPassToHz = 350.0f},
                {.waveform = Sine, .fromHz = 95.0f, .toHz = 38.0f, .gain = 0.9f, .decay = 0.3f},
                {.waveform = Square, .fromHz = 62.0f, .toHz = 48.0f, .gain = 0.25f, .decay = 0.1f}},
               14));
    // A throat: two saws a fifth apart with a slow waver, through a dark filter, over breath.
    set(SoundId::BossRoar,
        Recipe(1.9f, 3.0f,
               {{.waveform = Saw, .fromHz = 96.0f, .toHz = 72.0f, .gain = 0.9f, .attack = 0.15f, .decay = 0.9f,
                 .vibratoHz = 7.0f, .vibratoDepth = 0.05f, .lowPassFromHz = 1200.0f, .lowPassToHz = 500.0f},
                {.waveform = Saw, .fromHz = 144.0f, .toHz = 108.0f, .gain = 0.45f, .attack = 0.2f, .decay = 0.8f,
                 .vibratoHz = 5.5f, .vibratoDepth = 0.04f, .lowPassFromHz = 1500.0f, .lowPassToHz = 600.0f},
                {.waveform = Noise, .gain = 0.5f, .attack = 0.1f, .decay = 0.8f, .lowPassFromHz = 900.0f,
                 .lowPassToHz = 400.0f}},
               15));
    // The bar runs out: a heavy drop and a groan as she starts to change.
    set(SoundId::BossCollapse,
        Recipe(1.4f, 2.4f,
               {{.waveform = Sine, .fromHz = 90.0f, .toHz = 32.0f, .gain = 1.0f, .decay = 0.35f},
                {.waveform = Saw, .fromHz = 70.0f, .toHz = 50.0f, .gain = 0.5f, .attack = 0.2f, .decay = 0.5f,
                 .vibratoHz = 4.0f, .vibratoDepth = 0.06f, .lowPassFromHz = 600.0f, .lowPassToHz = 250.0f},
                {.waveform = Noise, .gain = 0.6f, .decay = 0.25f, .lowPassFromHz = 1200.0f, .lowPassToHz = 150.0f}},
               16));
    // A long wail that sinks: the voice of the roar falling away.
    set(SoundId::BossDeath,
        Recipe(2.6f, 2.6f,
               {{.waveform = Saw, .fromHz = 130.0f, .toHz = 32.0f, .gain = 0.9f, .attack = 0.05f, .decay = 1.0f,
                 .vibratoHz = 9.0f, .vibratoDepth = 0.07f, .lowPassFromHz = 1800.0f, .lowPassToHz = 250.0f},
                {.waveform = Saw, .fromHz = 196.0f, .toHz = 47.0f, .gain = 0.4f, .attack = 0.05f, .decay = 0.8f,
                 .vibratoHz = 6.0f, .vibratoDepth = 0.05f, .lowPassFromHz = 2000.0f, .lowPassToHz = 300.0f},
                {.waveform = Noise, .gain = 0.6f, .decay = 0.7f, .lowPassFromHz = 1500.0f, .lowPassToHz = 120.0f},
                {.waveform = Sine, .fromHz = 70.0f, .toHz = 28.0f, .gain = 0.8f, .decay = 0.5f}},
               17));
    // The warnings. Each must be told from the others with the eyes shut.
    // A low growl that rises as she crouches.
    set(SoundId::ChargeGrowl,
        Recipe(0.9f, 2.6f,
               {{.waveform = Saw, .fromHz = 55.0f, .toHz = 80.0f, .gain = 1.0f, .attack = 0.15f, .decay = 0.45f,
                 .vibratoHz = 13.0f, .vibratoDepth = 0.09f, .lowPassFromHz = 400.0f, .lowPassToHz = 900.0f},
                {.waveform = Noise, .gain = 0.35f, .attack = 0.1f, .decay = 0.4f, .lowPassFromHz = 300.0f,
                 .lowPassToHz = 700.0f}},
               18));
    // A hiss that swells, like breath drawn in before the spit.
    set(SoundId::RingSwell,
        Recipe(0.65f, 1.0f,
               {{.waveform = Noise, .gain = 1.0f, .attack = 0.45f, .decay = 0.08f, .lowPassFromHz = 300.0f,
                 .lowPassToHz = 3500.0f},
                {.waveform = Sine, .fromHz = 180.0f, .toHz = 520.0f, .gain = 0.25f, .attack = 0.4f,
                 .decay = 0.1f}},
               19));
    // A short, heavy grunt as she gathers herself to jump.
    set(SoundId::LeapGrunt,
        Recipe(0.5f, 2.4f,
               {{.waveform = Saw, .fromHz = 110.0f, .toHz = 65.0f, .gain = 0.8f, .attack = 0.06f, .decay = 0.18f,
                 .lowPassFromHz = 700.0f, .lowPassToHz = 300.0f},
                {.waveform = Sine, .fromHz = 130.0f, .toHz = 60.0f, .gain = 0.7f, .attack = 0.05f,
                 .decay = 0.2f}},
               20));
    // A quick rattle, like a snake: three bursts are coming.
    set(SoundId::VolleyHiss,
        Recipe(0.5f, 1.4f,
               {{.waveform = Noise, .gain = 1.0f, .attack = 0.03f, .decay = 0.25f, .vibratoHz = 0.0f,
                 .lowPassFromHz = 4000.0f, .lowPassToHz = 2000.0f},
                {.waveform = Square, .fromHz = 22.0f, .toHz = 30.0f, .gain = 0.5f, .attack = 0.03f,
                 .decay = 0.25f}},
               21));
    // A rising howl: her children are coming.
    set(SoundId::SummonHowl,
        Recipe(1.2f, 1.8f,
               {{.waveform = Sine, .fromHz = 260.0f, .toHz = 420.0f, .gain = 0.8f, .attack = 0.3f, .decay = 0.5f,
                 .vibratoHz = 5.0f, .vibratoDepth = 0.06f},
                {.waveform = Saw, .fromHz = 130.0f, .toHz = 210.0f, .gain = 0.4f, .attack = 0.3f, .decay = 0.5f,
                 .vibratoHz = 5.0f, .vibratoDepth = 0.06f, .lowPassFromHz = 700.0f, .lowPassToHz = 1200.0f}},
               22));
    // A wet "pfft".
    set(SoundId::Spit,
        Recipe(0.2f, 1.3f,
               {{.waveform = Noise, .gain = 1.0f, .attack = 0.004f, .decay = 0.05f, .lowPassFromHz = 5000.0f,
                 .lowPassToHz = 1400.0f},
                {.waveform = Sine, .fromHz = 900.0f, .toHz = 280.0f, .gain = 0.2f, .decay = 0.04f}},
               23));
    return r;
}

} // namespace

const SoundRecipe& RecipeFor(SoundId id) {
    static const std::array<SoundRecipe, kSoundCount> kRecipes = MakeRecipes();
    return kRecipes[static_cast<std::size_t>(id)];
}

} // namespace horde
