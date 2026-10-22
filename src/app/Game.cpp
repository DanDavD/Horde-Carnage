#include "app/Game.h"

#include <cmath>
#include <numbers>

#include <raylib.h>

#include "app/PlayerInput.h"
#include "app/RaylibConvert.h"

namespace horde {

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

// The arena is bigger than the screen, so the fight has room to move around and the camera has a job.
// At zoom 1 one world unit is one pixel, so the view is the screen size.
constexpr Arena kArena{{0.0f, 0.0f}, {2400.0f, 1600.0f}};
constexpr Vec2 kViewSize{static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight)};
constexpr float kGridSpacing = 80.0f;
constexpr float kWallThickness = 4.0f;
constexpr float kPlayerOutline = 3.0f;
// The eye sits toward the facing direction, so the player reads as pointing somewhere.
constexpr float kEyeOffset = 0.55f;
constexpr float kEyeRadius = 0.28f;
// Hairballs: a round body with short tufts around it, and two eyes that look at the player.
constexpr int kHairTufts = 12;
constexpr float kTuftLength = 0.35f;
constexpr float kHairballEyeOffset = 0.45f;
constexpr float kHairballEyeSpread = 0.35f;
constexpr float kHairballEyeRadius = 0.22f;

constexpr Color kBackground{20, 18, 24, 255};
constexpr Color kFloor{34, 30, 40, 255};
constexpr Color kGrid{44, 40, 52, 255};
constexpr Color kWall{110, 96, 120, 255};
constexpr Color kPlayerFill{90, 190, 230, 255};
constexpr Color kPlayerEdge{20, 60, 80, 255};
constexpr Color kHairballFill{150, 92, 70, 255};
constexpr Color kHairballEdge{70, 40, 32, 255};
constexpr Color kHairballEye{235, 225, 200, 255};
constexpr Color kSoftText{170, 165, 180, 255};

#ifndef NDEBUG
constexpr const char* kControlsHint = "Move: WASD, arrows, left stick or d-pad    Debug overlay: F1    Quit: Esc";
#else
constexpr const char* kControlsHint = "Move: WASD, arrows, left stick or d-pad    Quit: Esc";
#endif

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

void DrawPlayer(const Player& player, float alpha) {
    // Drawn between the last two fixed steps, so movement is smooth above 60 Hz.
    const Vec2 position = Lerp(player.motion.previousPosition, player.motion.position, alpha);
    DrawCircleV(ToRaylib(position), player.radius, kPlayerEdge);
    DrawCircleV(ToRaylib(position), player.radius - kPlayerOutline, kPlayerFill);
    DrawCircleV(ToRaylib(position + player.facing * (player.radius * kEyeOffset)), player.radius * kEyeRadius,
                kPlayerEdge);
}

void DrawHairball(const Enemy& enemy, Vec2 playerPosition, float alpha) {
    const Vec2 position = Lerp(enemy.motion.previousPosition, enemy.motion.position, alpha);
    const float radius = enemy.radius;

    for (int i = 0; i < kHairTufts; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / kHairTufts;
        const Vec2 out{std::cos(angle), std::sin(angle)};
        const Vec2 root = position + out * (radius * 0.7f);
        const Vec2 tip = position + out * (radius * (1.0f + kTuftLength));
        DrawLineEx(ToRaylib(root), ToRaylib(tip), 3.0f, kHairballEdge);
    }
    DrawCircleV(ToRaylib(position), radius, kHairballEdge);
    DrawCircleV(ToRaylib(position), radius - 3.0f, kHairballFill);

    // The eyes look at the player, so it is clear what the hairball is after even when it stands still.
    const Vec2 look = Normalized(playerPosition - position);
    const Vec2 side{-look.y, look.x};
    const Vec2 eyes = position + look * (radius * kHairballEyeOffset);
    DrawCircleV(ToRaylib(eyes + side * (radius * kHairballEyeSpread)), radius * kHairballEyeRadius, kHairballEye);
    DrawCircleV(ToRaylib(eyes - side * (radius * kHairballEyeSpread)), radius * kHairballEyeRadius, kHairballEye);
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

// Debug builds only, in screen space. The numbers I tune movement against: what the input asks for,
// what the body does, where the camera is, and how many enemies there are.
void DrawDebugHud(const World& world, Vec2 moveInput) {
    const Player& player = world.player;
    const FollowCamera& camera = world.camera;
    const Motion& motion = player.motion;
    constexpr int kX = 10;
    constexpr int kY = 36;
    constexpr int kLine = 20;
    DrawRectangle(kX - 4, kY - 4, 300, 7 * kLine + 8, Fade(BLACK, 0.6f));
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
    DrawText(TextFormat("enemies %d", static_cast<int>(world.enemies.size())), kX, kY + 6 * kLine, 18, RAYWHITE);
}
#endif

} // namespace

Game::Game() : _world{CreateWorld(kArena, kViewSize)} {
    // Vsync caps how often we render. How often the game simulates comes from FixedTimestep.
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "Horde Carnage");
}

Game::~Game() {
    CloseWindow();
}

void Game::Run() {
    while (!WindowShouldClose()) {
#ifndef NDEBUG
        // A key press is read once per frame, not per fixed step: a frame with no step would miss it.
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
    // Reading the hardware is the only part of a step that lives in the app. The rules are all in StepWorld.
    const Vec2 moveInput = ReadMoveInput();
    StepWorld(_world, moveInput, dt);
#ifndef NDEBUG
    _lastMoveInput = moveInput;
#endif
}

void Game::Draw(float alpha) const {
    BeginDrawing();
    ClearBackground(kBackground);

    const Player& player = _world.player;
    const Vec2 playerPosition = Lerp(player.motion.previousPosition, player.motion.position, alpha);

    BeginMode2D(ToRaylibCamera(_world.camera, alpha));
    DrawArena(_world.arena);
    for (const Enemy& enemy : _world.enemies) {
        DrawHairball(enemy, playerPosition, alpha);
    }
    // The player is drawn last, so it is never hidden under a crowd.
    DrawPlayer(player, alpha);
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugWorld(player, _world.camera, alpha);
    }
#endif
    EndMode2D();

    // Everything below is in screen space and does not move with the camera.
#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugHud(_world, _lastMoveInput);
    }
#endif

    DrawFPS(10, 10);
    DrawText("Horde Carnage v" HORDE_VERSION, kScreenWidth - 230, 10, 20, kSoftText);
    DrawText(kControlsHint, 40, kScreenHeight - 32, 20, kSoftText);
    EndDrawing();
}

} // namespace horde
