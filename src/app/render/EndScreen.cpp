#include "app/render/EndScreen.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

#include <raylib.h>

#include "app/render/Hud.h"
#include "app/render/Palette.h"
#include "app/render/View.h"

namespace horde {

namespace {

// The end screens: a dark band across the middle of the screen, with soft top and bottom edges.
constexpr int kEndBandTop = kScreenHeight / 2 - 120;
constexpr int kEndBandHeight = 220;
constexpr int kEndBandFeather = 48;

void DrawCenteredText(const char* text, int y, int fontSize, Color color) {
    const int width = MeasureText(text, fontSize);
    DrawText(text, (kScreenWidth - width) / 2, y, fontSize, color);
}

// What killed the player, for the death screen: knowing which attack did it is how the fight is learned.
const char* DeathCause(std::optional<DamageSource> source) {
    if (!source) {
        return "Slain";
    }
    switch (*source) {
    case DamageSource::BossTouch:
        return "Slain by her touch";
    case DamageSource::Charge:
        return "Slain by her Charge";
    case DamageSource::HairRing:
        return "Slain by her Hair Ring";
    case DamageSource::Volley:
        return "Slain by her Volley";
    case DamageSource::LeapImpact:
        return "Slain by her Leap";
    case DamageSource::Shockwave:
        return "Slain by her shockwave";
    case DamageSource::Frenzy:
        return "Slain by her frenzy";
    case DamageSource::Hairball:
        return "Slain by a hairball";
    }
    return "Slain";
}

// Minutes and seconds, the way the length of a fight is read.
std::string FormatDuration(float seconds) {
    const int whole = static_cast<int>(seconds);
    return TextFormat("%d:%02d", whole / 60, whole % 60);
}

// Rounded up, so a boss with any health left never reads 0%.
int HealthPercent(const Health& health) {
    return (health.current * 100 + health.max - 1) / health.max;
}

} // namespace

// The end of a fight, in the style of the games it borrows from: the world dims and a dark band across
// the middle carries one big word. It all fades in over the restart delay, so the moment the prompt lights
// up is the moment a press works.
void DrawEndScreen(const Session& session) {
    const float shown = std::min(session.stateSeconds / kRestartDelaySeconds, 1.0f);
    const bool won = session.state == SessionState::Victory;
    const World& world = session.world;

    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(BLACK, 0.35f * shown));
    const Color band = Fade(BLACK, 0.75f * shown);
    const Color clear = Fade(BLACK, 0.0f);
    DrawRectangleGradientV(0, kEndBandTop - kEndBandFeather, kScreenWidth, kEndBandFeather, clear, band);
    DrawRectangle(0, kEndBandTop, kScreenWidth, kEndBandHeight, band);
    DrawRectangleGradientV(0, kEndBandTop + kEndBandHeight, kScreenWidth, kEndBandFeather, band, clear);

    DrawCenteredText(won ? "VICTORY" : "YOU DIED", kEndBandTop + 28, 80, Fade(won ? kVictoryGold : kBlood, shown));

    const std::string duration = FormatDuration(world.elapsedSeconds);
    std::string summary;
    std::string detail;
    if (won) {
        const int accuracy = static_cast<int>(std::lround(Accuracy(world) * 100.0f));
        summary = TextFormat("You defeated %s on attempt %d, in %s.", kBossName, session.attempt, duration.c_str());
        detail = TextFormat("%d of %d shots on target (%d%%)", world.shotsHit, world.shotsFired, accuracy);
    } else {
        summary =
            TextFormat("Attempt %d. %s after %s.", session.attempt, DeathCause(world.lastHitBy), duration.c_str());
        detail = TextFormat("%s kept %d%% of her health.", kBossName, HealthPercent(world.boss.health));
    }
    DrawCenteredText(summary.c_str(), kEndBandTop + 124, 26, Fade(RAYWHITE, shown));
    DrawCenteredText(detail.c_str(), kEndBandTop + 154, 22, Fade(kSoftText, shown));

    // Dimmed until the restart delay has passed, so it is clear why an early press did nothing.
    const Color prompt = CanRestart(session) ? RAYWHITE : Fade(kSoftText, 0.4f * shown);
    const char* again = won ? "Press Enter or R (A or Start on a gamepad) to fight her again"
                            : "Press Enter or R (A or Start on a gamepad) to try again";
    // Inside the band, so it never lands on top of the player, who stands in the middle of the screen.
    DrawCenteredText(again, kEndBandTop + 192, 22, prompt);
}

} // namespace horde
