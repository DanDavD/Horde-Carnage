#pragma once

// Only compiled into Debug builds; Release builds never call these.

#include <cstddef>

#include <optional>

#include "app/audio/MusicPlayer.h"
#include "core/FollowCamera.h"
#include "core/GamepadType.h"
#include "core/Player.h"
#include "core/Vec2.h"
#include "core/World.h"

namespace horde {

/// Debug builds only. In world space: the player's velocity and facing, and where the camera is centred.
void DrawDebugWorld(const Player& player, const FollowCamera& camera, float alpha);

/// Debug builds only. In game pixels: the numbers I tune against, from the input to the boss's state and the
/// music's layers.
void DrawDebugHud(const World& world, Vec2 moveInput, std::size_t particleCount,
                  std::optional<int> gamepadSlot, GamepadType gamepadType, const char* gamepadName,
                  const MusicPlayer& music);

} // namespace horde
