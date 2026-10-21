#include "app/Game.h"

#include <raylib.h>

#include "app/PlayerInput.h"
#include "app/RaylibConvert.h"
#include "core/Arena.h"
#include "core/Facing.h"

namespace horde {

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

// The arena fits the screen for now, so no camera is needed yet.
constexpr Arena kArena{{40.0f, 60.0f}, {1240.0f, 680.0f}};
constexpr float kGridSpacing = 80.0f;
constexpr float kWallThickness = 4.0f;
constexpr float kPlayerOutline = 3.0f;
// The eye sits toward the facing direction, so the player reads as pointing somewhere.
constexpr float kEyeOffset = 0.55f;
constexpr float kEyeRadius = 0.28f;

constexpr Color kBackground{20, 18, 24, 255};
constexpr Color kFloor{34, 30, 40, 255};
constexpr Color kGrid{44, 40, 52, 255};
constexpr Color kWall{110, 96, 120, 255};
constexpr Color kPlayerFill{90, 190, 230, 255};
constexpr Color kPlayerEdge{20, 60, 80, 255};
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

#ifndef NDEBUG
// Debug builds only. The numbers I tune movement against: what the input asks for, and what the body does.
void DrawDebugOverlay(const Player& player, Vec2 moveInput, float alpha) {
    const Motion& motion = player.motion;
    const Vec2 position = Lerp(motion.previousPosition, motion.position, alpha);

    // Velocity in green (a quarter second ahead), facing in yellow.
    DrawLineEx(ToRaylib(position), ToRaylib(position + motion.velocity * 0.25f), 2.0f, GREEN);
    DrawLineEx(ToRaylib(position), ToRaylib(position + player.facing * (player.radius * 2.0f)), 2.0f, YELLOW);

    constexpr int kX = 10;
    constexpr int kY = 36;
    constexpr int kLine = 20;
    DrawRectangle(kX - 4, kY - 4, 300, 5 * kLine + 8, Fade(BLACK, 0.6f));
    DrawText(TextFormat("input  %5.2f %5.2f  len %4.2f", moveInput.x, moveInput.y, Length(moveInput)), kX, kY,
             18, RAYWHITE);
    DrawText(TextFormat("speed  %5.0f / %.0f", Length(motion.velocity), player.stats.maxSpeed), kX, kY + kLine, 18,
             RAYWHITE);
    DrawText(TextFormat("vel    %5.0f %5.0f", motion.velocity.x, motion.velocity.y), kX, kY + 2 * kLine, 18,
             RAYWHITE);
    DrawText(TextFormat("facing %5.2f %5.2f", player.facing.x, player.facing.y), kX, kY + 3 * kLine, 18, RAYWHITE);
    DrawText(TextFormat("pos    %5.0f %5.0f", motion.position.x, motion.position.y), kX, kY + 4 * kLine, 18,
             RAYWHITE);
}
#endif

} // namespace

Game::Game() : _player{.motion = MotionAt(Center(kArena))} {
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
    const Vec2 moveInput = ReadMoveInput();
    UpdateFacing(_player.facing, moveInput);
    StepMovement(_player.motion, moveInput, _player.stats, dt);
    ConfineToArena(_player.motion, _player.radius, kArena);
#ifndef NDEBUG
    _lastMoveInput = moveInput;
#endif
}

void Game::Draw(float alpha) const {
    BeginDrawing();
    ClearBackground(kBackground);

    DrawArena(kArena);
    DrawPlayer(_player, alpha);

#ifndef NDEBUG
    if (_showDebugOverlay) {
        DrawDebugOverlay(_player, _lastMoveInput, alpha);
    }
#endif

    DrawFPS(10, 10);
    DrawText("Horde Carnage v" HORDE_VERSION, kScreenWidth - 230, 10, 20, kSoftText);
    DrawText(kControlsHint, 40, kScreenHeight - 32, 20, kSoftText);
    EndDrawing();
}

} // namespace horde
