#include "app/Game.h"

#include <cmath>
#include <cstddef>
#include <numbers>
#include <string>

#include <raylib.h>

#include "app/PlayerInput.h"
#include "app/RaylibConvert.h"

namespace horde {

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

// At zoom 1 one world unit is one pixel, so the view is the screen size.
constexpr Vec2 kViewSize{static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight)};
constexpr float kGridSpacing = 80.0f;
constexpr float kWallThickness = 4.0f;
constexpr float kPlayerOutline = 3.0f;
// The eye sits toward the facing direction, so the player reads as pointing somewhere.
constexpr float kEyeOffset = 0.55f;
constexpr float kEyeRadius = 0.28f;
// Hairballs and the boss: a round body with short tufts around it, and two eyes that look at the player.
// Sizes are fractions of the body radius, so the boss is the same drawing, scaled up.
constexpr float kTuftLength = 0.35f;
constexpr float kHairballEyeOffset = 0.45f;
constexpr float kHairballEyeSpread = 0.35f;
constexpr float kHairballEyeRadius = 0.22f;
// The boss's health bar runs along the bottom of the screen, under its name, above the controls line.
constexpr const char* kBossName = "The Matriarch";
constexpr int kBossBarWidth = 640;
constexpr int kBossBarHeight = 12;
constexpr int kBossBarY = kScreenHeight - 96;
// While invulnerable the player blinks: faded for one period, solid for the next.
constexpr float kBlinkPeriod = 0.1f;
constexpr float kBlinkFade = 0.35f;
// The gun is a barrel that sticks out of the body toward the aim, as long as `WeaponStats::barrelLength`.
// The SMG's is thicker, so it reads as the bigger gun.
constexpr float kPistolBarrelWidth = 6.0f;
constexpr float kSmgBarrelWidth = 9.0f;
constexpr float kMuzzleFlashRadius = 8.0f;
// The dash leaves a fading trail of ghosts between where it started and the body, so a move that takes
// seven frames still reads as a jump.
constexpr int kDashGhosts = 4;
// The dash bar sits under the health bar.
constexpr int kDashBarY = 64;
constexpr int kDashBarHeight = 6;
// Flask icons sit to the right of the health bar; a drink shows as a ring filling around the player.
constexpr int kFlaskIconX = 282;
constexpr int kFlaskIconWidth = 12;
constexpr int kFlaskIconHeight = 18;
constexpr int kFlaskIconGap = 6;
constexpr float kDrinkRingGap = 4.0f;
constexpr float kDrinkRingWidth = 4.0f;
// A bullet is drawn as a short streak behind its head, so its direction reads even in a still frame.
constexpr float kBulletStreak = 14.0f;
constexpr float kBulletWidth = 4.0f;
// Weapon panel in the bottom right corner, one box per slot.
constexpr int kSlotBoxWidth = 150;
constexpr int kSlotBoxHeight = 48;
constexpr int kSlotBoxGap = 8;
constexpr int kHudMargin = 12;

constexpr Color kBackground{20, 18, 24, 255};
constexpr Color kFloor{34, 30, 40, 255};
constexpr Color kGrid{44, 40, 52, 255};
constexpr Color kWall{110, 96, 120, 255};
constexpr Color kPlayerFill{90, 190, 230, 255};
constexpr Color kPlayerEdge{20, 60, 80, 255};
constexpr Color kPlayerDeadFill{95, 95, 105, 255};
constexpr Color kPlayerDeadEdge{45, 45, 52, 255};
constexpr Color kHairballFill{150, 92, 70, 255};
constexpr Color kHairballEdge{70, 40, 32, 255};
constexpr Color kHairballEye{235, 225, 200, 255};
// The boss is darker and redder than its children, with eyes that glow.
constexpr Color kBossFill{112, 58, 64, 255};
constexpr Color kBossEdge{48, 24, 30, 255};
constexpr Color kBossEye{255, 190, 90, 255};
constexpr Color kBossBar{170, 32, 40, 255};
constexpr Color kSoftText{170, 165, 180, 255};
constexpr Color kBlood{220, 60, 50, 255};
constexpr Color kBarBack{50, 20, 24, 255};
constexpr Color kBarHurt{255, 140, 120, 255};
constexpr Color kBarEdge{120, 100, 110, 255};
// Light steel, so the barrel stands out against the dark floor where it leaves the body.
constexpr Color kGun{175, 180, 195, 255};
constexpr Color kMuzzleFlash{255, 220, 120, 255};
constexpr Color kCrosshair{240, 240, 245, 255};
constexpr Color kBullet{255, 230, 140, 255};
constexpr Color kFlask{245, 150, 45, 255};
constexpr Color kHitFlash{255, 255, 255, 255};

// How a hairy body is drawn. `flash` is how far a hit turns it toward white: all the way for a hairball,
// which dies in a few hits, but only part of the way for the boss, which the SMG hits ten times a second
// and which would otherwise stay white for the whole fight.
struct HairStyle {
    Color fill;
    Color edge;
    Color eye;
    int tufts;
    float tuftWidth;
    float outline;
    float flash;
};

constexpr HairStyle kHairballStyle{kHairballFill, kHairballEdge, kHairballEye, 12, 3.0f, 3.0f, 1.0f};
constexpr HairStyle kBossStyle{kBossFill, kBossEdge, kBossEye, 28, 6.0f, 5.0f, 0.45f};
// A dead boss turns grey like a dead player, and its eyes go dark.
constexpr HairStyle kDeadBossStyle{kPlayerDeadFill, kPlayerDeadEdge, kPlayerDeadEdge, 28, 6.0f, 5.0f, 0.0f};

#ifndef NDEBUG
constexpr const char* kControlsHint =
    "WASD move  Mouse aim  Click fire  Space dash  E heal  Q/1/2/wheel swap gun  Esc quit  F1 debug";
#else
constexpr const char* kControlsHint =
    "WASD move  Mouse aim  Click fire  Space dash  E heal  Q/1/2/wheel swap gun  Esc quit";
#endif

const char* FireModeLabel(FireMode mode) {
    return mode == FireMode::Auto ? "AUTO" : "SEMI-AUTO";
}

void DrawCenteredText(const char* text, int y, int fontSize, Color color) {
    const int width = MeasureText(text, fontSize);
    DrawText(text, (kScreenWidth - width) / 2, y, fontSize, color);
}

void DrawArena(const Arena& arena) {
    const Vec2 size = arena.max - arena.min;
    DrawRectangleV(ToRaylib(arena.min), ToRaylib(size), kFloor);

    // The grid gives the eye something fixed to judge speed against.
    for (float x = arena.min.x + kGridSpacing; x < arena.max.x; x += kGridSpacing) {
        DrawLineV(ToRaylib({x, arena.min.y}), ToRaylib({x, arena.max.y}), kGrid);
    }
    for (float y = arena.min.y + kGridSpacing; y < arena.max.y; y += kGridSpacing) {
        DrawLineV(ToRaylib({arena.min.x, y}), ToRaylib({arena.max.x, y}), kGrid);
    }

    // raylib draws the outline inward, so the rectangle grows by the wall thickness. Otherwise a mover
    // confined to the arena would overlap the wall.
    const Rectangle walls{arena.min.x - kWallThickness, arena.min.y - kWallThickness,
                          size.x + 2.0f * kWallThickness, size.y + 2.0f * kWallThickness};
    DrawRectangleLinesEx(walls, kWallThickness, kWall);
}

// Ghosts fade with the dash's invulnerability, so the trail also shows how long the player is safe.
void DrawDashTrail(const Player& player, Vec2 position) {
    const Dash& dash = player.dash;
    if (!IsDashInvulnerable(dash)) {
        return;
    }
    const float strength = dash.invulnerableSeconds / dash.stats.invulnerability;
    for (int i = 0; i < kDashGhosts; ++i) {
        const float along = static_cast<float>(i) / kDashGhosts;
        const Vec2 ghost = Lerp(dash.origin, position, along);
        const float size = player.radius * (0.6f + 0.4f * along);
        DrawCircleV(ToRaylib(ghost), size, Fade(kPlayerFill, 0.4f * strength * along));
    }
}

// A ring that fills clockwise while the player drinks, so both the player and anyone watching can see
// how long they are exposed.
void DrawDrinkRing(const Player& player, Vec2 position) {
    const Flasks& flasks = player.flasks;
    if (!IsDrinking(flasks)) {
        return;
    }
    const float progress = 1.0f - flasks.drinkingSeconds / flasks.stats.drinkSeconds;
    const float inner = player.radius + kDrinkRingGap;
    DrawRing(ToRaylib(position), inner, inner + kDrinkRingWidth, -90.0f, -90.0f + 360.0f * progress, 32, kFlask);
}

void DrawPlayer(const Player& player, float alpha) {
    // Drawn between the last two fixed steps, so movement is smooth above 60 Hz.
    const Vec2 position = Lerp(player.motion.previousPosition, player.motion.position, alpha);
    const Health& health = player.health;
    DrawDashTrail(player, position);
    DrawDrinkRing(player, position);

    Color edge = IsDead(health) ? kPlayerDeadEdge : kPlayerEdge;
    Color fill = IsDead(health) ? kPlayerDeadFill : kPlayerFill;
    Color gun = IsDead(health) ? kPlayerDeadEdge : kGun;
    // The blink tells the player they were hit and that the next touch will not hurt yet.
    const bool fadedHalf = static_cast<int>(health.invulnerableSeconds / kBlinkPeriod) % 2 == 1;
    if (IsInvulnerable(health) && fadedHalf) {
        edge = Fade(edge, kBlinkFade);
        fill = Fade(fill, kBlinkFade);
        gun = Fade(gun, kBlinkFade);
    }

    // The barrel goes under the body, so it reads as held, sticking out toward the aim.
    const Weapon& weapon = Equipped(player.loadout);
    const bool automatic = weapon.stats.mode == FireMode::Auto;
    const Vec2 muzzle = MuzzlePoint(position, player.facing, player.radius, weapon.stats);
    DrawLineEx(ToRaylib(position), ToRaylib(muzzle), automatic ? kSmgBarrelWidth : kPistolBarrelWidth, gun);

    DrawCircleV(ToRaylib(position), player.radius, edge);
    DrawCircleV(ToRaylib(position), player.radius - kPlayerOutline, fill);
    DrawCircleV(ToRaylib(position + player.facing * (player.radius * kEyeOffset)), player.radius * kEyeRadius, edge);

    if (weapon.muzzleFlashSeconds > 0.0f) {
        DrawCircleV(ToRaylib(muzzle), kMuzzleFlashRadius, kMuzzleFlash);
    }
}

Color Mix(Color from, Color to, float amount) {
    const auto channel = [amount](unsigned char a, unsigned char b) {
        const float mixed = static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * amount;
        return static_cast<unsigned char>(mixed);
    };
    return Color{channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b), channel(from.a, to.a)};
}

void DrawHairBody(Vec2 position, float radius, Vec2 lookAt, const HairStyle& style, bool flashing) {
    // A hit turns the body toward white for a moment, so every bullet that lands is visible.
    const float flash = flashing ? style.flash : 0.0f;
    const Color edge = Mix(style.edge, kHitFlash, flash);
    const Color fill = Mix(style.fill, kHitFlash, flash);

    for (int i = 0; i < style.tufts; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(style.tufts);
        const Vec2 out{std::cos(angle), std::sin(angle)};
        const Vec2 root = position + out * (radius * 0.7f);
        const Vec2 tip = position + out * (radius * (1.0f + kTuftLength));
        DrawLineEx(ToRaylib(root), ToRaylib(tip), style.tuftWidth, edge);
    }
    DrawCircleV(ToRaylib(position), radius, edge);
    DrawCircleV(ToRaylib(position), radius - style.outline, fill);

    // The eyes look at the player, so it is clear what the body is after even when it stands still.
    const Vec2 look = Normalized(lookAt - position);
    const Vec2 side{-look.y, look.x};
    const Vec2 eyes = position + look * (radius * kHairballEyeOffset);
    DrawCircleV(ToRaylib(eyes + side * (radius * kHairballEyeSpread)), radius * kHairballEyeRadius, style.eye);
    DrawCircleV(ToRaylib(eyes - side * (radius * kHairballEyeSpread)), radius * kHairballEyeRadius, style.eye);
}

void DrawHairball(const Enemy& enemy, Vec2 playerPosition, float alpha) {
    const Vec2 position = Lerp(enemy.motion.previousPosition, enemy.motion.position, alpha);
    DrawHairBody(position, enemy.radius, playerPosition, kHairballStyle, enemy.hitFlashSeconds > 0.0f);
}

void DrawBoss(const Boss& boss, Vec2 playerPosition, float alpha) {
    const Vec2 position = Lerp(boss.motion.previousPosition, boss.motion.position, alpha);
    if (IsDead(boss.health)) {
        DrawHairBody(position, boss.radius, playerPosition, kDeadBossStyle, false);
        return;
    }
    DrawHairBody(position, boss.radius, playerPosition, kBossStyle, boss.hitFlashSeconds > 0.0f);
}

void DrawBullet(const Projectile& bullet, float alpha) {
    const Vec2 head = Lerp(bullet.motion.previousPosition, bullet.motion.position, alpha);
    const Vec2 tail = head - Normalized(bullet.motion.velocity) * kBulletStreak;
    DrawLineEx(ToRaylib(tail), ToRaylib(head), kBulletWidth, kBullet);
    DrawCircleV(ToRaylib(head), bullet.radius, kBullet);
}

void DrawHealthBar(const Health& health) {
    constexpr int kX = 10;
    constexpr int kY = 38;
    constexpr int kWidth = 260;
    constexpr int kHeight = 22;
    const float fraction = static_cast<float>(health.current) / static_cast<float>(health.max);

    DrawRectangle(kX, kY, kWidth, kHeight, kBarBack);
    // The fill turns pale while the player is invulnerable, so a hit reads on the bar as well as on the body.
    DrawRectangle(kX, kY, static_cast<int>(kWidth * fraction), kHeight, IsInvulnerable(health) ? kBarHurt : kBlood);
    DrawRectangleLines(kX, kY, kWidth, kHeight, kBarEdge);
    DrawText(TextFormat("%d / %d", health.current, health.max), kX + 8, kY + 2, 18, RAYWHITE);
}

// Wide and thin along the bottom of the screen, with the name above it, so it reads as the one fight that
// matters and stays out of the way of the arena.
void DrawBossBar(const Boss& boss) {
    constexpr int kX = (kScreenWidth - kBossBarWidth) / 2;
    const float fraction = static_cast<float>(boss.health.current) / static_cast<float>(boss.health.max);
    DrawText(kBossName, kX, kBossBarY - 24, 20, RAYWHITE);
    DrawRectangle(kX, kBossBarY, kBossBarWidth, kBossBarHeight, kBarBack);
    DrawRectangle(kX, kBossBarY, static_cast<int>(kBossBarWidth * fraction), kBossBarHeight, kBossBar);
    DrawRectangleLines(kX, kBossBarY, kBossBarWidth, kBossBarHeight, kBarEdge);
}

// One box per slot. The gun in hand is lit, so the player can tell at a glance which one they hold.
void DrawWeaponPanel(const Loadout& loadout) {
    const int top = kScreenHeight - kSlotBoxHeight - kHudMargin;
    for (int slot = 0; slot < kWeaponSlots; ++slot) {
        const Weapon& weapon = loadout.slots[static_cast<std::size_t>(slot)];
        const bool inHand = slot == loadout.equipped;
        const int slotsToTheRight = kWeaponSlots - slot;
        const int left = kScreenWidth - kHudMargin - slotsToTheRight * (kSlotBoxWidth + kSlotBoxGap) + kSlotBoxGap;
        const Rectangle box{static_cast<float>(left), static_cast<float>(top), static_cast<float>(kSlotBoxWidth),
                            static_cast<float>(kSlotBoxHeight)};

        DrawRectangleRec(box, inHand ? Fade(kPlayerFill, 0.25f) : Fade(BLACK, 0.45f));
        DrawRectangleLinesEx(box, inHand ? 2.0f : 1.0f, inHand ? kPlayerFill : kBarEdge);
        const Color text = inHand ? RAYWHITE : kSoftText;
        const std::string name = TextToUpper(std::string(weapon.stats.name).c_str());
        DrawText(TextFormat("%d  %s", slot + 1, name.c_str()), left + 10, top + 6, 20, text);
        DrawText(FireModeLabel(weapon.stats.mode), left + 10, top + 28, 14, text);
    }
}

// Drawn in place of the system cursor while playing, so the aim point is easy to find in a crowd.
void DrawCrosshair(Vector2 at) {
    constexpr float kRadius = 10.0f;
    constexpr float kTick = 6.0f;
    DrawCircleLinesV(at, kRadius, kCrosshair);
    DrawLineEx({at.x - kRadius - kTick, at.y}, {at.x - kRadius + 3.0f, at.y}, 2.0f, kCrosshair);
    DrawLineEx({at.x + kRadius - 3.0f, at.y}, {at.x + kRadius + kTick, at.y}, 2.0f, kCrosshair);
    DrawLineEx({at.x, at.y - kRadius - kTick}, {at.x, at.y - kRadius + 3.0f}, 2.0f, kCrosshair);
    DrawLineEx({at.x, at.y + kRadius - 3.0f}, {at.x, at.y + kRadius + kTick}, 2.0f, kCrosshair);
    DrawCircleV(at, 1.5f, kCrosshair);
}

// One bottle per charge, lit while it is full.
void DrawFlaskIcons(const Flasks& flasks) {
    for (int i = 0; i < flasks.stats.maxCharges; ++i) {
        const float x = static_cast<float>(kFlaskIconX + i * (kFlaskIconWidth + kFlaskIconGap));
        const Rectangle bottle{x, 41.0f, static_cast<float>(kFlaskIconWidth), static_cast<float>(kFlaskIconHeight)};
        const Rectangle neck{x + 3.0f, 37.0f, static_cast<float>(kFlaskIconWidth) - 6.0f, 5.0f};
        const Color fill = i < flasks.charges ? kFlask : kBarBack;
        DrawRectangleRec(neck, kBarEdge);
        DrawRectangleRounded(bottle, 0.5f, 6, fill);
        DrawRectangleRoundedLinesEx(bottle, 0.5f, 6, 1.0f, kBarEdge);
    }
}

// Fills up as the dash recharges, and lights up when it is ready.
void DrawDashBar(const Dash& dash) {
    constexpr int kX = 10;
    constexpr int kWidth = 260;
    const float total = dash.stats.duration + dash.stats.cooldown;
    const float ready = 1.0f - dash.cooldownSeconds / total;
    DrawRectangle(kX, kDashBarY, kWidth, kDashBarHeight, kBarBack);
    DrawRectangle(kX, kDashBarY, static_cast<int>(kWidth * ready), kDashBarHeight,
                  dash.cooldownSeconds > 0.0f ? kSoftText : kPlayerFill);
}

void DrawGameOver(const Session& session) {
    DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(BLACK, 0.55f));
    DrawCenteredText("GAME OVER", kScreenHeight / 2 - 90, 64, kBlood);
    const World& world = session.world;
    const char* summary = TextFormat("You lasted %.1f seconds. %s had %d of %d health left.", world.elapsedSeconds,
                                     kBossName, world.boss.health.current, world.boss.health.max);
    DrawCenteredText(summary, kScreenHeight / 2 - 10, 28, RAYWHITE);
    // Dimmed until the restart delay has passed, so it is clear why an early press did nothing.
    const Color prompt = CanRestart(session) ? RAYWHITE : Fade(kSoftText, 0.4f);
    DrawCenteredText("Press Enter or R (A or Start on a gamepad) to play again", kScreenHeight / 2 + 40, 22, prompt);
}

Camera2D ToRaylibCamera(const FollowCamera& camera, float alpha) {
    // The camera is interpolated like the player. If only the player were, the two would drift apart
    // by a fraction of a step every frame and the player would shimmer against the floor.
    const Vec2 center = Lerp(camera.previousCenter, camera.center, alpha);
    return Camera2D{.offset = {kScreenWidth * 0.5f, kScreenHeight * 0.5f},
                    .target = ToRaylib(center),
                    .rotation = 0.0f,
                    .zoom = 1.0f};
}

#ifndef NDEBUG
// Debug builds only. Drawn in world space: velocity in green (a quarter second ahead), facing in yellow,
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

// Debug builds only, in screen space. The numbers I tune against: what the input asks for, what the
// body does, where the camera is, how many enemies there are, and the player's health window.
void DrawDebugHud(const World& world, Vec2 moveInput) {
    const Player& player = world.player;
    const FollowCamera& camera = world.camera;
    const Motion& motion = player.motion;
    constexpr int kX = 10;
    constexpr int kY = 80; // Below the health and dash bars.
    constexpr int kLine = 20;
    DrawRectangle(kX - 4, kY - 4, 300, 13 * kLine + 8, Fade(BLACK, 0.6f));
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
    DrawText(TextFormat("enemies %d  bullets %d", static_cast<int>(world.enemies.Size()),
                        static_cast<int>(world.bullets.Size())),
             kX, kY + 6 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("health %d / %d  inv %.2f", player.health.current, player.health.max,
                        player.health.invulnerableSeconds),
             kX, kY + 7 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("time   %.2f s", world.elapsedSeconds), kX, kY + 8 * kLine, 18, RAYWHITE);
    const Weapon& weapon = Equipped(player.loadout);
    DrawText(TextFormat("weapon %s  cd %.2f", std::string(weapon.stats.name).c_str(), weapon.cooldownSeconds), kX,
             kY + 9 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("shots  %d  boss %d / %d", world.shotsFired, world.boss.health.current, world.boss.health.max),
             kX, kY + 10 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("dash   cd %.2f  inv %.2f", player.dash.cooldownSeconds, player.dash.invulnerableSeconds), kX,
             kY + 11 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("flasks %d / %d  drink %.2f", player.flasks.charges, player.flasks.stats.maxCharges,
                        player.flasks.drinkingSeconds),
             kX, kY + 12 * kLine, 18, RAYWHITE);
}
#endif

} // namespace

Game::Game() : _session{StartSession(kBossArena, kViewSize)} {
    // Vsync caps how often we render. How often the game simulates comes from FixedTimestep.
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "Horde Carnage");
}

Game::~Game() {
    CloseWindow();
}

void Game::Run() {
    while (!WindowShouldClose()) {
        // Key presses are read once per frame, not per fixed step: a frame with no step would miss them.
        if (ReadRestartPressed()) {
            TryRestart(_session);
        }
        CollectButtonPresses(_pendingPresses);

        // The crosshair replaces the system cursor while playing. On the game over screen the cursor is back.
        const bool playing = _session.state == SessionState::Playing;
        if (playing && !IsCursorHidden()) {
            HideCursor();
        } else if (!playing && IsCursorHidden()) {
            ShowCursor();
        }
#ifndef NDEBUG
        if (IsKeyPressed(KEY_F1)) {
            _showDebugOverlay = !_showDebugOverlay;
        }
#endif
        const int steps = _timestep.Advance(GetFrameTime());
        for (int i = 0; i < steps; ++i) {
            Update(static_cast<float>(_timestep.StepSeconds()));
        }
        Draw(static_cast<float>(_timestep.Alpha()));
    }
}

void Game::Update(float dt) {
    // Reading the hardware is the only part of a step that lives in the app. The rules are all in
    // StepSession and StepWorld.
    //
    // The mouse is turned into a world point with the camera as of the last step. The camera drawn on
    // screen is a fraction of a step behind that, which is far below a pixel of aiming error.
    const Camera2D camera = ToRaylibCamera(_session.world.camera, 1.0f);
    const PlayerCommands commands{.move = ReadMoveInput(),
                                  .aimAt = FromRaylib(GetScreenToWorld2D(GetMousePosition(), camera)),
                                  .fireHeld = ReadFireHeld(),
                                  .firePressed = _pendingPresses.fire,
                                  .selectSlot = _pendingPresses.weaponSlot,
                                  .cycleWeapon = _pendingPresses.cycleWeapon,
                                  .dashPressed = _pendingPresses.dash,
                                  .healPressed = _pendingPresses.heal};
    // Presses are events: only the first step of a frame uses them, so a frame that runs two steps does
    // not fire twice for one click.
    _pendingPresses = {};
    StepSession(_session, commands, dt);
#ifndef NDEBUG
    _lastMoveInput = commands.move;
#endif
}

void Game::Draw(float alpha) const {
    BeginDrawing();
    ClearBackground(kBackground);

    const World& world = _session.world;
    const bool playing = _session.state == SessionState::Playing;
    // A frozen world has no next step to blend toward. Interpolating it would make everything shake
    // between its last two positions as alpha changes from frame to frame, so it is drawn where it stopped.
    if (!playing) {
        alpha = 1.0f;
    }

    const Player& player = world.player;
    const Vec2 playerPosition = Lerp(player.motion.previousPosition, player.motion.position, alpha);

    BeginMode2D(ToRaylibCamera(world.camera, alpha));
    DrawArena(world.arena);
    // The boss goes under its children, so a dead boss lies on the floor.
    DrawBoss(world.boss, playerPosition, alpha);
    for (const Enemy& enemy : world.enemies) {
        DrawHairball(enemy, playerPosition, alpha);
    }
    for (const Projectile& bullet : world.bullets) {
        DrawBullet(bullet, alpha);
    }
    // The player is drawn last, so it is never hidden under a crowd.
    DrawPlayer(player, alpha);
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugWorld(player, world.camera, alpha);
    }
#endif
    EndMode2D();

    // Everything below is in screen space and does not move with the camera.
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugHud(world, _lastMoveInput);
    }
#endif

    DrawHealthBar(player.health);
    DrawDashBar(player.dash);
    DrawFlaskIcons(player.flasks);
    DrawWeaponPanel(player.loadout);
    DrawBossBar(world.boss);
    DrawFPS(10, 10);
    DrawText("Horde Carnage v" HORDE_VERSION, kScreenWidth - 230, 10, 20, kSoftText);
    if (playing) {
        DrawText(kControlsHint, kHudMargin, kScreenHeight - 30, 18, kSoftText);
        DrawCrosshair(GetMousePosition());
    } else {
        DrawGameOver(_session);
    }
    EndDrawing();
}

} // namespace horde
