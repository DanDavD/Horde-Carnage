#include "app/render/DebugOverlay.h"

#include <algorithm>
#include <string>

#include <raylib.h>

#include "app/RaylibConvert.h"
#include "core/BossTheme.h"
#include "core/Loadout.h"

#ifndef NDEBUG

namespace horde {

namespace {

const char* AttackKindLabel(AttackKind kind) {
    switch (kind) {
    case AttackKind::Charge:
        return "charge";
    case AttackKind::HairRing:
        return "ring";
    case AttackKind::Leap:
        return "leap";
    case AttackKind::Volley:
        return "volley";
    case AttackKind::Summon:
        return "summon";
    }
    return "";
}

const char* AttackStageLabel(AttackStage stage) {
    switch (stage) {
    case AttackStage::Warning:
        return "warning";
    case AttackStage::Active:
        return "active";
    case AttackStage::Recovery:
        return "recovery";
    }
    return "";
}

// What the boss is doing, and for how long, so attack timings can be tuned against numbers.
const char* BossStateLabel(const Boss& boss) {
    if (IsDead(boss.health)) {
        return "dead";
    }
    if (IsTransforming(boss)) {
        return TextFormat("transforming %.2f", boss.transformSecondsLeft);
    }
    const char* phase = boss.phase == BossPhase::One ? "p1" : (boss.frenzied ? "p2 frenzy" : "p2");
    if (!boss.attack) {
        return TextFormat("%s stalk, attack in %.2f", phase, boss.secondsUntilAttack);
    }
    const BossAttack& attack = *boss.attack;
    const char* combo = attack.followUp ? " combo 1/2" : (attack.endsCombo ? " combo 2/2" : "");
    return TextFormat("%s %s %s %.2f/%.2f%s%s%s", phase, AttackKindLabel(attack.kind), AttackStageLabel(attack.stage),
                      attack.stageSeconds, WarningSeconds(boss, attack), attack.stunned ? " stunned" : "",
                      IsAirborne(boss) ? " airborne" : "", combo);
}

// The shield's kind and strength, or how long the boss stays exposed, and the stagger.
const char* ShieldLabel(const Boss& boss) {
    const Shield& shield = boss.shield;
    if (!IsShieldUp(shield)) {
        return TextFormat("shield exposed %.2f  stagger %.2f", shield.exposedSecondsLeft,
                          std::max(boss.staggerSeconds, 0.0f));
    }
    return TextFormat("shield %s %.0f / %.0f", shield.kind == ShieldKind::Shell ? "shell" : "barrier",
                      shield.strength, MaxStrength(shield.stats, shield.kind));
}

// How likely each attack is at the player's distance right now, and the one the next pick skips.
const char* AttackPickLabel(const World& world) {
    const Boss& boss = world.boss;
    const float distance = Length(world.player.motion.position - boss.motion.position);
    const AttackChoiceStats& choice = boss.choice;
    return TextFormat("pick %3.0f c%.1f r%.1f l%.1f v%.1f s%.1f not %s", distance,
                      AttackWeightAt(choice, AttackKind::Charge, distance),
                      AttackWeightAt(choice, AttackKind::HairRing, distance),
                      AttackWeightAt(choice, AttackKind::Leap, distance),
                      AttackWeightAt(choice, AttackKind::Volley, distance),
                      AttackWeightAt(choice, AttackKind::Summon, distance),
                      boss.lastAttack ? AttackKindLabel(*boss.lastAttack) : "-");
}

} // namespace

// Drawn in world space: velocity in green (a quarter second ahead), facing in yellow,
// and a cross where the camera is centred.
void DrawDebugWorld(const Player& player, const FollowCamera& camera, float alpha) {
    const Motion& motion = player.motion;
    const Vec2 position = Lerp(motion.previousPosition, motion.position, alpha);
    DrawLineEx(ToRaylib(position), ToRaylib(position + motion.velocity * 0.25f), 2.0f, GREEN);
    DrawLineEx(ToRaylib(position), ToRaylib(position + player.facing * (player.radius * 2.0f)), 2.0f, YELLOW);

    const Vec2 center = Lerp(camera.previousCenter, camera.center, alpha);
    DrawLineEx(ToRaylib(center + Vec2{-8.0f, 0.0f}), ToRaylib(center + Vec2{8.0f, 0.0f}), 2.0f, ORANGE);
    DrawLineEx(ToRaylib(center + Vec2{0.0f, -8.0f}), ToRaylib(center + Vec2{0.0f, 8.0f}), 2.0f, ORANGE);
}

// In screen space. The numbers I tune against: what the input asks for, what the
// body does, where the camera is, how many enemies and projectiles there are, the player's health window,
// and what the boss is doing.
void DrawDebugHud(const World& world, Vec2 moveInput, std::size_t particleCount,
                  std::optional<int> gamepadSlot, GamepadType gamepadType, const char* gamepadName,
                  const MusicPlayer& music) {
    const Player& player = world.player;
    const FollowCamera& camera = world.camera;
    const Motion& motion = player.motion;
    constexpr int kX = 10;
    constexpr int kY = 80; // Below the health and dash bars.
    constexpr int kLine = 20;
    DrawRectangle(kX - 4, kY - 4, 460, 19 * kLine + 8, Fade(BLACK, 0.6f));
    DrawText(TextFormat("input  %5.2f %5.2f  len %4.2f", moveInput.x, moveInput.y, Length(moveInput)), kX, kY,
             18, RAYWHITE);
    DrawText(TextFormat("speed  %5.0f / %.0f", Length(motion.velocity), player.stats.maxSpeed), kX, kY + kLine, 18,
             RAYWHITE);
    DrawText(TextFormat("vel    %5.0f %5.0f", motion.velocity.x, motion.velocity.y), kX, kY + 2 * kLine, 18,
             RAYWHITE);
    DrawText(TextFormat("facing %5.2f %5.2f", player.facing.x, player.facing.y), kX, kY + 3 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("pos    %5.0f %5.0f", motion.position.x, motion.position.y), kX, kY + 4 * kLine, 18,
             RAYWHITE);
    DrawText(TextFormat("camera %5.0f %5.0f", camera.center.x, camera.center.y), kX, kY + 5 * kLine, 18, ORANGE);
    DrawText(TextFormat("enemies %d  bullets %d  hair %d", static_cast<int>(world.enemies.Size()),
                        static_cast<int>(world.bullets.Size()), static_cast<int>(world.hostileProjectiles.Size())),
             kX, kY + 6 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("health %d / %d  inv %.2f", player.health.current, player.health.max,
                        player.health.invulnerableSeconds),
             kX, kY + 7 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("time   %.2f s", world.elapsedSeconds), kX, kY + 8 * kLine, 18, RAYWHITE);
    const Weapon& weapon = Equipped(player.loadout);
    DrawText(TextFormat("weapon %s  cd %.2f", std::string(weapon.stats.name).c_str(), weapon.cooldownSeconds), kX,
             kY + 9 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("shots  %d hit %d  boss %d / %d", world.shotsFired, world.shotsHit, world.boss.health.current,
                        world.boss.health.max),
             kX, kY + 10 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("dash   cd %.2f  inv %.2f", player.dash.cooldownSeconds, player.dash.invulnerableSeconds), kX,
             kY + 11 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("flasks %d / %d  drink %.2f", player.flasks.charges, player.flasks.stats.maxCharges,
                        player.flasks.drinkingSeconds),
             kX, kY + 12 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("boss   %s", BossStateLabel(world.boss)), kX, kY + 13 * kLine, 18, ORANGE);
    DrawText(TextFormat("seed   %08X  waves %d  marks %d  part %d", static_cast<unsigned>(world.seed),
                        static_cast<int>(world.shockwaves.Size()), static_cast<int>(world.spawnMarkers.Size()),
                        static_cast<int>(particleCount)),
             kX, kY + 14 * kLine, 18, RAYWHITE);
    DrawText(AttackPickLabel(world), kX, kY + 15 * kLine, 18, ORANGE);
    DrawText(ShieldLabel(world.boss), kX, kY + 16 * kLine, 18, ORANGE);
    if (gamepadSlot.has_value()) {
        DrawText(TextFormat("pad    #%d %s (%.22s)", *gamepadSlot, GamepadTypeName(gamepadType), gamepadName), kX,
                 kY + 17 * kLine, 18, RAYWHITE);
    } else {
        DrawText("pad    none", kX, kY + 17 * kLine, 18, RAYWHITE);
    }
    // What the music is playing: the volume, each layer (pulse, hymn, wrath, desperation, hook), the bar, and
    // a stinger while one plays.
    const MusicMixer& mixer = music.mixer;
    const std::size_t barFrames = std::max<std::size_t>(music.track.loopFrames / kBossThemeBars, 1);
    const std::size_t stingerFrames = mixer.stinger == MusicStinger::Victory ? music.track.victory.size()
                                      : mixer.stinger == MusicStinger::Defeat ? music.track.defeat.size()
                                                                              : 0;
    const char* stinger = mixer.stingerPosition >= stingerFrames        ? "-"
                          : mixer.stinger == MusicStinger::Victory ? "victory"
                                                                    : "defeat";
    DrawText(TextFormat("music  %.2f p%.1f h%.1f w%.1f d%.1f k%.1f bar %d %s %.2f", mixer.volume, mixer.gains[0],
                        mixer.gains[1], mixer.gains[2], mixer.gains[3], mixer.gains[4],
                        static_cast<int>(mixer.position / barFrames) + 1, stinger, mixer.stingerGain),
             kX, kY + 18 * kLine, 18, music.ready ? ORANGE : GRAY);
}

} // namespace horde

#endif
