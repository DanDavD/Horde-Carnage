#include "app/Game.h"

#include <raylib.h>

#include "app/PlayerInput.h"
#include "app/RaylibConvert.h"
#include "core/Arena.h"

namespace horde {

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

// The arena fits the screen for now, so no camera is needed yet.
constexpr Arena kArena{{40.0f, 60.0f}, {1240.0f, 680.0f}};
constexpr float kGridSpacing = 80.0f;
constexpr float kWallThickness = 4.0f;
constexpr float kPlayerOutline = 3.0f;

constexpr Color kBackground{20, 18, 24, 255};
constexpr Color kFloor{34, 30, 40, 255};
constexpr Color kGrid{44, 40, 52, 255};
constexpr Color kWall{110, 96, 120, 255};
constexpr Color kPlayerFill{90, 190, 230, 255};
constexpr Color kPlayerEdge{20, 60, 80, 255};
constexpr Color kSoftText{170, 165, 180, 255};

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
}

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
        const int steps = _timestep.Advance(GetFrameTime());
        for (int i = 0; i < steps; ++i) {
            Update(static_cast<float>(_timestep.StepSeconds()));
        }
        Draw(static_cast<float>(_timestep.Alpha()));
    }
}

void Game::Update(float dt) {
    StepMovement(_player.motion, ReadMoveInput(), _player.stats, dt);
    ConfineToArena(_player.motion, _player.radius, kArena);
}

void Game::Draw(float alpha) const {
    BeginDrawing();
    ClearBackground(kBackground);

    DrawArena(kArena);
    DrawPlayer(_player, alpha);

    DrawFPS(10, 10);
    DrawText("Horde Carnage v" HORDE_VERSION, kScreenWidth - 230, 10, 20, kSoftText);
    DrawText("Move: WASD, arrows or left stick    Quit: Esc", 40, kScreenHeight - 32, 20, kSoftText);
    EndDrawing();
}

} // namespace horde
