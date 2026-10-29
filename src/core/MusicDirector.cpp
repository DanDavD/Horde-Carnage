#include "core/MusicDirector.h"

#include <algorithm>
#include <cstddef>

namespace horde {

namespace {

// 0 above `from`, 1 at `full` and below, a straight line between.
float Ramp(float value, float from, float full) {
    if (from == full) {
        return value <= full ? 1.0f : 0.0f;
    }
    return std::clamp((from - value) / (from - full), 0.0f, 1.0f);
}

// What each layer would be at, if nothing held it up.
std::array<float, kMusicStemCount> RawLevels(const MusicDirectorStats& stats, const MusicSituation& situation) {
    std::array<float, kMusicStemCount> raw{};
    raw[static_cast<std::size_t>(MusicStem::Pulse)] = 1.0f;
    raw[static_cast<std::size_t>(MusicStem::Hymn)] = situation.bossPhase == BossPhase::Two ? 1.0f : 0.0f;
    float wrath = 0.0f;
    if (situation.bossFrenzied) {
        wrath = 1.0f;
    } else if (situation.bossPhase != BossPhase::Transforming) {
        wrath = Ramp(situation.bossHealthFraction, stats.wrathFromFraction, stats.wrathFullFraction);
    }
    raw[static_cast<std::size_t>(MusicStem::Wrath)] = wrath;
    raw[static_cast<std::size_t>(MusicStem::Desperation)] =
        Ramp(static_cast<float>(situation.playerHealth), static_cast<float>(stats.desperationFromHealth),
             static_cast<float>(stats.desperationFullHealth));
    return raw;
}

} // namespace

MusicSituation SituationOf(const Session& session) {
    const Boss& boss = session.world.boss;
    const float max = static_cast<float>(std::max(boss.health.max, 1));
    return MusicSituation{.state = session.state,
                          .bossPhase = boss.phase,
                          .bossFrenzied = boss.frenzied,
                          .bossHealthFraction = static_cast<float>(boss.health.current) / max,
                          .playerHealth = session.world.player.health.current};
}

MusicCue StepMusicDirector(MusicDirector& director, const MusicSituation& situation, float dt) {
    const MusicDirectorStats& stats = director.stats;
    MusicCue cue;
    const bool inFight = situation.state == SessionState::Playing || situation.state == SessionState::Paused;
    // A new fight, or the second phase coming in: the theme starts over, and whatever came before, a low bar or
    // a dying player in the last fight, the first phase's Wrath, is not held over into it.
    cue.restart = inFight && (!director.inFight || (director.bossPhase == BossPhase::Transforming &&
                                                    situation.bossPhase == BossPhase::Two));
    director.inFight = inFight;
    director.bossPhase = situation.bossPhase;

    if (inFight) {
        const std::array<float, kMusicStemCount> raw = RawLevels(stats, situation);
        if (cue.restart) {
            director.levels = raw;
            director.holdSecondsLeft.fill(stats.holdSeconds);
        }
        for (std::size_t i = 0; i < kMusicStemCount; ++i) {
            float& level = director.levels[i];
            float& hold = director.holdSecondsLeft[i];
            if (raw[i] >= level) {
                level = raw[i];
                hold = stats.holdSeconds;
            } else {
                hold = std::max(hold - dt, 0.0f);
                if (hold == 0.0f) {
                    level = raw[i];
                }
            }
        }
    }
    cue.stems = director.levels;

    switch (situation.state) {
    case SessionState::Playing:
        cue.volume = situation.bossPhase == BossPhase::Transforming ? stats.transformingVolume : 1.0f;
        cue.volumeSeconds = stats.fightVolumeSeconds;
        break;
    case SessionState::Paused:
        cue.volume = stats.pausedVolume;
        cue.volumeSeconds = stats.pauseVolumeSeconds;
        break;
    case SessionState::Dead:
        cue.volumeSeconds = stats.deathVolumeSeconds;
        break;
    case SessionState::Victory:
        cue.volumeSeconds = stats.victoryVolumeSeconds;
        break;
    case SessionState::Title:
        cue.volumeSeconds = stats.titleVolumeSeconds;
        break;
    }
    return cue;
}

} // namespace horde
