#include "app/render/EndScreen.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

#include <raylib.h>

#include "app/render/Hud.h"
#include "app/render/MenuDraw.h"
#include "app/render/Palette.h"
#include "app/render/Text.h"
#include "app/render/View.h"

namespace horde {

namespace {

// The end screens: a dark band across the middle of the screen, with soft top and bottom edges.
constexpr int kEndBandTop = kScreenHeight / 2 - 120;
constexpr int kEndBandHeight = 220;
constexpr int kEndBandFeather = 48;

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
// the middle carries one big word. It all fades in over the restart delay, so the moment the choices light
// up is the moment a press works. After a victory it waits for the boss to finish dying first.
void DrawEndScreen(const Session& session, const Menu& menu, float sinceMoved, const Art& art) {
    const float shown = std::clamp(EndScreenSeconds(session) / kRestartDelaySeconds, 0.0f, 1.0f);
    if (shown <= 0.0f) {
        return;
    }
    const bool won = session.state == SessionState::Victory;
    const World& world = session.world;

    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(BLACK, 0.35f * shown));
    const Color band = Fade(BLACK, 0.75f * shown);
    const Color clear = Fade(BLACK, 0.0f);
    DrawRectangleGradientV(0, kEndBandTop - kEndBandFeather, kScreenWidth, kEndBandFeather, clear, band);
    DrawRectangle(0, kEndBandTop, kScreenWidth, kEndBandHeight, band);
    DrawRectangleGradientV(0, kEndBandTop + kEndBandHeight, kScreenWidth, kEndBandFeather, band, clear);

    // The one big word, in the serif of old inscriptions, spaced wide.
    constexpr float kMiddle = kScreenWidth / 2.0f;
    constexpr auto kTop = static_cast<float>(kEndBandTop);
    DrawGameTextCentered(art.titleFont, won ? "VICTORY" : "YOU DIED", kMiddle, kTop + 22.0f, 88.0f,
                         Fade(won ? kVictoryGold : kBlood, shown), kTitleTracking);

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
    DrawGameTextCentered(art.titleFont, summary.c_str(), kMiddle, kTop + 122.0f, 26.0f, Fade(kText, shown));
    DrawGameTextCentered(art.titleFont, detail.c_str(), kMiddle, kTop + 154.0f, 20.0f, Fade(kSoftText, shown));

    // Inside the band, so they never land on top of the player, who stands in the middle of the screen, and
    // dimmed until the restart delay has passed, so it is clear why an early press did nothing.
    const float ready = CanRestart(session) ? 1.0f : 0.35f * shown;
    DrawMenu(MenuLayoutFor(session.state), MenuLabelsFor(session.state), menu, sinceMoved, ready, art);
}

} // namespace horde
