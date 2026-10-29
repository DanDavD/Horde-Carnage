#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include <doctest/doctest.h>

#include "core/BossTheme.h"

using horde::MusicStem;
using horde::MusicTrack;

namespace {

// Low enough to compose quickly, high enough for every note of the theme.
constexpr int kRate = 11025;

const MusicTrack& Theme() {
    static const MusicTrack theme = horde::ComposeBossTheme(kRate);
    return theme;
}

const std::vector<float>& Stem(MusicStem stem) {
    return Theme().stems[static_cast<std::size_t>(stem)];
}

float Peak(const std::vector<float>& samples, std::size_t from, std::size_t to) {
    float peak = 0.0f;
    for (std::size_t i = from; i < std::min(to, samples.size()); ++i) {
        peak = std::max(peak, std::abs(samples[i]));
    }
    return peak;
}

} // namespace

TEST_CASE("a sixteenth note of the theme lands on a whole frame at the game's rate") {
    CHECK(horde::BossThemeStepFrame(1, 44100) == 4410u);
    CHECK(horde::BossThemeStepFrame(horde::kStepsPerBar, 44100) == 70560u);
}

TEST_CASE("the theme is sixteen bars at 150 beats a minute, and every layer fills the loop") {
    // 16 bars of 4 beats at 0.4 s a beat.
    CHECK(Theme().loopFrames == static_cast<std::size_t>(25.6 * kRate));
    CHECK(Theme().sampleRate == kRate);
    for (const std::vector<float>& stem : Theme().stems) {
        CHECK(stem.size() == Theme().loopFrames);
    }
}

TEST_CASE("every layer of the theme plays, and all four together never clip") {
    for (const std::vector<float>& stem : Theme().stems) {
        CHECK(Peak(stem, 0, stem.size()) > 0.05f);
    }
    float loudest = 0.0f;
    for (std::size_t i = 0; i < Theme().loopFrames; ++i) {
        float sum = 0.0f;
        for (const std::vector<float>& stem : Theme().stems) {
            sum += stem[i];
        }
        loudest = std::max(loudest, std::abs(sum));
    }
    CHECK(loudest == doctest::Approx(0.89f).epsilon(0.001));
}

TEST_CASE("the Pulse hits on every beat of the loop, but for the two bars where the band stops") {
    const std::vector<float>& pulse = Stem(MusicStem::Pulse);
    const float peak = Peak(pulse, 0, pulse.size());
    const auto window = static_cast<std::size_t>(0.03 * kRate);
    for (int beat = 0; beat < horde::kBossThemeBars * 4; ++beat) {
        if ((beat / 4) % 8 == 7) {
            continue;
        }
        const std::size_t start = horde::BossThemeStepFrame(beat * 4, kRate);
        CHECK(Peak(pulse, start, start + window) > 0.25f * peak);
    }
}

TEST_CASE("the band stops in the last bar of each phrase") {
    const std::vector<float>& pulse = Stem(MusicStem::Pulse);
    const auto rms = [&](int fromStep, int toStep) {
        const std::size_t from = horde::BossThemeStepFrame(fromStep, kRate);
        const std::size_t to = horde::BossThemeStepFrame(toStep, kRate);
        double sum = 0.0;
        for (std::size_t i = from; i < to; ++i) {
            sum += static_cast<double>(pulse[i]) * pulse[i];
        }
        return std::sqrt(sum / static_cast<double>(to - from));
    };
    for (const int bar : {7, 15}) {
        const int start = bar * horde::kStepsPerBar;
        const int before = start - horde::kStepsPerBar;
        // The beat after the last stab, against the same beat of the bar before, where the band plays on. Only
        // the crash and the last stab ring on into it.
        CHECK(rms(start + 8, start + 12) < 0.6 * rms(before + 8, before + 12));
    }
}

TEST_CASE("the Hook plays through both phrases and holds its breath with the band at the end") {
    const std::vector<float>& hook = Stem(MusicStem::Hook);
    const std::size_t half = Theme().loopFrames / 2;
    CHECK(Peak(hook, 0, half) > 0.05f);
    CHECK(Peak(hook, half, hook.size()) > 0.05f);
    // The second half of the last bar is a rest.
    const int lastBar = (horde::kBossThemeBars - 1) * horde::kStepsPerBar;
    CHECK(Peak(hook, horde::BossThemeStepFrame(lastBar + 9, kRate), Theme().loopFrames) < 0.001f);
}

TEST_CASE("the victory and the defeat open on their blow, peak like the theme and die away to silence") {
    const std::vector<float>& victory = Theme().victory;
    const std::vector<float>& defeat = Theme().defeat;
    CHECK(victory.size() == static_cast<std::size_t>(10.5f * kRate));
    CHECK(defeat.size() == static_cast<std::size_t>(6.5f * kRate));
    for (const std::vector<float>* stinger : {&victory, &defeat}) {
        const float peak = Peak(*stinger, 0, stinger->size());
        CHECK(peak == doctest::Approx(0.89f).epsilon(0.001));
        CHECK(Peak(*stinger, 0, static_cast<std::size_t>(0.05 * kRate)) > 0.4f * peak);
        CHECK(Peak(*stinger, stinger->size() - static_cast<std::size_t>(0.1 * kRate), stinger->size()) < 0.02f);
    }
}

TEST_CASE("the Desperation's heart beats a sixteenth after every beat, between the kicks") {
    const std::vector<float>& desperation = Stem(MusicStem::Desperation);
    const auto window = static_cast<std::size_t>(0.05 * kRate);
    const std::size_t dub = horde::BossThemeStepFrame(1, kRate);
    const std::size_t silence = horde::BossThemeStepFrame(2, kRate) + window;
    // The dub, plus the trembling strings, is louder than the strings alone later in the beat.
    CHECK(Peak(desperation, dub, dub + window) > 1.5f * Peak(desperation, silence, silence + window));
}

TEST_CASE("the same rate always composes the same theme") {
    const MusicTrack first = horde::ComposeBossTheme(4000);
    const MusicTrack second = horde::ComposeBossTheme(4000);
    CHECK(first.stems == second.stems);
    CHECK(first.victory == second.victory);
    CHECK(first.defeat == second.defeat);
}
