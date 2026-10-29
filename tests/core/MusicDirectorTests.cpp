#include <cstddef>

#include <doctest/doctest.h>

#include "core/MusicDirector.h"
#include "core/World.h"

using horde::BossPhase;
using horde::MusicCue;
using horde::MusicDirector;
using horde::MusicSituation;
using horde::MusicStem;
using horde::SessionState;

namespace {

constexpr float kFrame = 1.0f / 60.0f;

MusicSituation Fight(int playerHealth = 100, float bossHealthFraction = 1.0f, BossPhase phase = BossPhase::One,
                     bool frenzied = false) {
    return MusicSituation{.state = SessionState::Playing,
                          .bossPhase = phase,
                          .bossFrenzied = frenzied,
                          .bossHealthFraction = bossHealthFraction,
                          .playerHealth = playerHealth};
}

MusicSituation In(SessionState state, MusicSituation situation = Fight()) {
    situation.state = state;
    return situation;
}

float Level(const MusicCue& cue, MusicStem stem) {
    return cue.stems[static_cast<std::size_t>(stem)];
}

} // namespace

TEST_CASE("a fight starts the theme over with the Pulse and the Hook, at full volume") {
    MusicDirector director;
    const MusicCue first = horde::StepMusicDirector(director, Fight(), kFrame);
    CHECK(first.restart);
    CHECK(first.volume == doctest::Approx(1.0f));
    CHECK(first.startStinger == horde::MusicStinger::None);
    CHECK(Level(first, MusicStem::Pulse) == doctest::Approx(1.0f));
    CHECK(Level(first, MusicStem::Hook) == doctest::Approx(1.0f));
    CHECK(Level(first, MusicStem::Hymn) == doctest::Approx(0.0f));
    CHECK(Level(first, MusicStem::Wrath) == doctest::Approx(0.0f));
    CHECK(Level(first, MusicStem::Desperation) == doctest::Approx(0.0f));

    const MusicCue second = horde::StepMusicDirector(director, Fight(), kFrame);
    CHECK_FALSE(second.restart);
}

TEST_CASE("the band drops out while the boss transforms, and the second phase slams in with the Hymn") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(), kFrame);
    const MusicCue roar =
        horde::StepMusicDirector(director, Fight(100, 1.0f, BossPhase::Transforming), kFrame);
    CHECK_FALSE(roar.restart);
    CHECK(roar.volume == doctest::Approx(director.stats.transformingVolume));

    const MusicCue two = horde::StepMusicDirector(director, Fight(100, 1.0f, BossPhase::Two), kFrame);
    CHECK(two.restart);
    CHECK(two.volume == doctest::Approx(1.0f));
    CHECK(Level(two, MusicStem::Hymn) == doctest::Approx(1.0f));
    CHECK(Level(two, MusicStem::Pulse) == doctest::Approx(1.0f));
}

TEST_CASE("the first phase's bar running low leaves the music alone") {
    MusicDirector director;
    const MusicCue low = horde::StepMusicDirector(director, Fight(100, 0.05f), kFrame);
    CHECK(Level(low, MusicStem::Wrath) == doctest::Approx(0.0f));
    CHECK(Level(low, MusicStem::Hook) == doctest::Approx(1.0f));
    (void)horde::StepMusicDirector(director, Fight(100, 1.0f, BossPhase::Transforming), kFrame);
    const MusicCue two = horde::StepMusicDirector(director, Fight(100, 1.0f, BossPhase::Two), kFrame);
    CHECK(Level(two, MusicStem::Wrath) == doctest::Approx(0.0f));
}

TEST_CASE("the Wrath comes in as the boss's second bar runs low") {
    MusicDirector director;
    const auto wrathAt = [&director](float fraction) {
        return Level(horde::StepMusicDirector(director, Fight(100, fraction, BossPhase::Two), kFrame),
                     MusicStem::Wrath);
    };
    CHECK(wrathAt(0.5f) == doctest::Approx(0.0f));
    CHECK(wrathAt(0.35f) == doctest::Approx(0.0f));
    CHECK(wrathAt(0.25f) == doctest::Approx(0.5f));
    CHECK(wrathAt(0.15f) == doctest::Approx(1.0f));
    CHECK(wrathAt(0.02f) == doctest::Approx(1.0f));
}

TEST_CASE("the Hook gives way as the Wrath comes in, without a dip in loudness") {
    MusicDirector director;
    const MusicCue half = horde::StepMusicDirector(director, Fight(100, 0.25f, BossPhase::Two), kFrame);
    const float wrath = Level(half, MusicStem::Wrath);
    const float hook = Level(half, MusicStem::Hook);
    CHECK(wrath == doctest::Approx(0.5f));
    // Equal power: the squares of the two gains add up to 1 all the way through the handover.
    CHECK(wrath * wrath + hook * hook == doctest::Approx(1.0f));
    const MusicCue full = horde::StepMusicDirector(director, Fight(100, 0.1f, BossPhase::Two), kFrame);
    CHECK(Level(full, MusicStem::Hook) == doctest::Approx(0.0f));
    // The Wrath held up while it waits out its hold keeps the Hook out with it.
    const MusicCue held = horde::StepMusicDirector(director, Fight(100, 0.9f, BossPhase::Two), 1.0f);
    CHECK(Level(held, MusicStem::Wrath) == doctest::Approx(1.0f));
    CHECK(Level(held, MusicStem::Hook) == doctest::Approx(0.0f));
}

TEST_CASE("the frenzy brings the Wrath all the way in at once") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(100, 0.6f, BossPhase::Two), kFrame);
    const MusicCue frenzy = horde::StepMusicDirector(director, Fight(100, 0.5f, BossPhase::Two, true), kFrame);
    CHECK(Level(frenzy, MusicStem::Wrath) == doctest::Approx(1.0f));
}

TEST_CASE("the Desperation comes in as the player's health runs low, all the way at 30") {
    MusicDirector director;
    CHECK(Level(horde::StepMusicDirector(director, Fight(60), kFrame), MusicStem::Desperation) ==
          doctest::Approx(0.0f));
    CHECK(Level(horde::StepMusicDirector(director, Fight(45), kFrame), MusicStem::Desperation) ==
          doctest::Approx(0.0f));
    CHECK(Level(horde::StepMusicDirector(director, Fight(38), kFrame), MusicStem::Desperation) ==
          doctest::Approx(7.0f / 15.0f));
    CHECK(Level(horde::StepMusicDirector(director, Fight(30), kFrame), MusicStem::Desperation) ==
          doctest::Approx(1.0f));
    CHECK(Level(horde::StepMusicDirector(director, Fight(5), kFrame), MusicStem::Desperation) ==
          doctest::Approx(1.0f));
}

TEST_CASE("a layer that came in waits out its hold before it goes down") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(30), kFrame);
    // A flask takes the player out of danger, but the music does not believe it straight away.
    CHECK(Level(horde::StepMusicDirector(director, Fight(70), 1.0f), MusicStem::Desperation) ==
          doctest::Approx(1.0f));
    CHECK(Level(horde::StepMusicDirector(director, Fight(70), 1.0f), MusicStem::Desperation) ==
          doctest::Approx(1.0f));
    CHECK(Level(horde::StepMusicDirector(director, Fight(70), 1.0f), MusicStem::Desperation) ==
          doctest::Approx(0.0f));
}

TEST_CASE("a layer going back up cancels its hold") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(30), kFrame);
    (void)horde::StepMusicDirector(director, Fight(70), 2.0f);
    (void)horde::StepMusicDirector(director, Fight(30), kFrame);
    // A fresh hold: two more seconds are not enough to take it down.
    CHECK(Level(horde::StepMusicDirector(director, Fight(70), 2.0f), MusicStem::Desperation) ==
          doctest::Approx(1.0f));
}

TEST_CASE("the pause turns the fight down without starting it over") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(), kFrame);
    const MusicCue paused = horde::StepMusicDirector(director, In(SessionState::Paused), kFrame);
    CHECK_FALSE(paused.restart);
    CHECK(paused.volume == doctest::Approx(director.stats.pausedVolume));
    CHECK(Level(paused, MusicStem::Pulse) == doctest::Approx(1.0f));

    const MusicCue resumed = horde::StepMusicDirector(director, Fight(), kFrame);
    CHECK_FALSE(resumed.restart);
    CHECK(resumed.volume == doctest::Approx(1.0f));
}

TEST_CASE("a death and a victory fade the theme out with its layers as they were") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(20), kFrame);
    const MusicCue dead = horde::StepMusicDirector(director, In(SessionState::Dead, Fight(0)), kFrame);
    CHECK(dead.volume == doctest::Approx(0.0f));
    CHECK(dead.volumeSeconds == doctest::Approx(director.stats.deathVolumeSeconds));
    CHECK(Level(dead, MusicStem::Desperation) == doctest::Approx(1.0f));

    MusicDirector won;
    (void)horde::StepMusicDirector(won, Fight(100, 0.05f, BossPhase::Two), kFrame);
    const MusicCue victory =
        horde::StepMusicDirector(won, In(SessionState::Victory, Fight(100, 0.0f, BossPhase::Two)), kFrame);
    CHECK(victory.volume == doctest::Approx(0.0f));
    CHECK(victory.volumeSeconds == doctest::Approx(won.stats.victoryVolumeSeconds));
    CHECK(Level(victory, MusicStem::Wrath) == doctest::Approx(1.0f));
}

TEST_CASE("a death starts the Defeat stinger once, and the theme gets out of its way") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(10), kFrame);
    const MusicCue dead = horde::StepMusicDirector(director, In(SessionState::Dead, Fight(0)), kFrame);
    CHECK(dead.startStinger == horde::MusicStinger::Defeat);
    CHECK(dead.stingerVolume == doctest::Approx(1.0f));
    CHECK(dead.volume == doctest::Approx(0.0f));
    const MusicCue later = horde::StepMusicDirector(director, In(SessionState::Dead, Fight(0)), kFrame);
    CHECK(later.startStinger == horde::MusicStinger::None);
    CHECK(later.stingerVolume == doctest::Approx(1.0f));
}

TEST_CASE("a victory starts the Victory stinger") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(100, 0.05f, BossPhase::Two), kFrame);
    const MusicCue won =
        horde::StepMusicDirector(director, In(SessionState::Victory, Fight(100, 0.0f, BossPhase::Two)), kFrame);
    CHECK(won.startStinger == horde::MusicStinger::Victory);
    CHECK(won.stingerVolume == doctest::Approx(1.0f));
}

TEST_CASE("a retry cuts the stinger short, and the title fades it out") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(10), kFrame);
    (void)horde::StepMusicDirector(director, In(SessionState::Dead, Fight(0)), kFrame);
    const MusicCue title = horde::StepMusicDirector(director, In(SessionState::Title), kFrame);
    CHECK(title.stingerVolume == doctest::Approx(0.0f));
    CHECK(title.stingerVolumeSeconds == doctest::Approx(director.stats.titleVolumeSeconds));

    MusicDirector again;
    (void)horde::StepMusicDirector(again, Fight(10), kFrame);
    (void)horde::StepMusicDirector(again, In(SessionState::Dead, Fight(0)), kFrame);
    const MusicCue retry = horde::StepMusicDirector(again, Fight(), kFrame);
    CHECK(retry.startStinger == horde::MusicStinger::None);
    CHECK(retry.stingerVolume == doctest::Approx(0.0f));
    CHECK(retry.stingerVolumeSeconds == doctest::Approx(again.stats.stingerCutSeconds));
}

TEST_CASE("a retry starts over and forgets how close the last fight came") {
    MusicDirector director;
    (void)horde::StepMusicDirector(director, Fight(10, 0.1f), kFrame);
    (void)horde::StepMusicDirector(director, In(SessionState::Dead, Fight(0, 0.1f)), kFrame);
    const MusicCue retry = horde::StepMusicDirector(director, Fight(), kFrame);
    CHECK(retry.restart);
    CHECK(Level(retry, MusicStem::Desperation) == doctest::Approx(0.0f));
    CHECK(Level(retry, MusicStem::Wrath) == doctest::Approx(0.0f));
}

TEST_CASE("the title screen is silent") {
    MusicDirector director;
    const MusicCue title = horde::StepMusicDirector(director, In(SessionState::Title), kFrame);
    CHECK_FALSE(title.restart);
    CHECK(title.volume == doctest::Approx(0.0f));
}

TEST_CASE("the music reads the boss's bar in progress and the player's health from the session") {
    horde::Session session = horde::StartSession(horde::kBossArena, {1280.0f, 720.0f});
    session.world.boss.health.current = session.world.boss.health.max / 4;
    session.world.player.health.current = 35;
    const MusicSituation situation = horde::SituationOf(session);
    CHECK(situation.state == SessionState::Playing);
    CHECK(situation.bossPhase == BossPhase::One);
    CHECK_FALSE(situation.bossFrenzied);
    CHECK(situation.bossHealthFraction == doctest::Approx(0.25f));
    CHECK(situation.playerHealth == 35);
}
