#pragma once

#include <span>

#include "core/Vec2.h"
#include "core/World.h"

namespace horde {

/// Fixed distance in world units where the gamepad crosshair sits in front of the player.
/// At 260 units it stays comfortably inside the 1280x720 view without crowding the hunter.
inline constexpr float kGamepadCrosshairDistance = 260.0f;

/// Tunable parameters for the soft gamepad aim assist.
struct AimAssistStats {
    float maxRange = 750.0f;        ///< Reach of the furthest gun; targets past this are ignored.
    float maxAngleRadians = 0.42f;  ///< Half-cone of ~24 degrees (48 degrees total).
    float maxStrength = 0.35f;      ///< Maximum pull toward target center (0 = off, 1 = snap).
    float crosshairDistance = kGamepadCrosshairDistance;
};

/// Persistent aim heading for a gamepad across fixed steps.
struct GamepadAim {
    Vec2 direction{0.0f, -1.0f};    ///< Unit aim vector; starts pointing north toward the Matriarch.
    Vec2 rawDirection{0.0f, -1.0f}; ///< Unassisted heading, kept so held fire does not compound aim assist.
};

/// Softly deflects an aim direction toward the best candidate among `targets`.
///
/// Returns `aimDirection` normalized (or unchanged if near zero) when no target lies within
/// `stats.maxRange` and within `stats.maxAngleRadians`. Inside the cone, the direction bends smoothly
/// toward the target closest in angle, tapering to zero pull at the cone's edge.
[[nodiscard]] Vec2 ApplyAimAssist(Vec2 playerPosition, Vec2 aimDirection,
                                  std::span<const Vec2> targets,
                                  const AimAssistStats& stats = {});

/// Gathers alive combat targets from `world` (the Matriarch while undefeated, plus living hairballs)
/// and applies soft aim assist to `aimDirection`.
[[nodiscard]] Vec2 AssistAimInWorld(const World& world, Vec2 aimDirection,
                                    const AimAssistStats& stats = {});

/// Works out the world-space aim point at `crosshairDistance` from `playerPosition`, using
/// `stickDirection` when non-zero or `fallbackDirection` otherwise.
[[nodiscard]] Vec2 GamepadAimPoint(Vec2 playerPosition, Vec2 stickDirection, Vec2 fallbackDirection,
                                   float crosshairDistance = kGamepadCrosshairDistance);

/// Updates `aim` for one fixed step and returns the world-space aim point. Only the right stick aims: the
/// left stick moves the hunter and never turns the aim.
/// - Tilting `rightStick` aims along the stick with soft aim assist.
/// - Releasing `rightStick` keeps the current heading. While `firing`, the soft assist is worked out again
///   from the unassisted heading, so it follows a target without compounding.
[[nodiscard]] Vec2 StepGamepadAim(GamepadAim& aim, const World& world, Vec2 rightStick, bool firing,
                                  const AimAssistStats& stats = {});

} // namespace horde
