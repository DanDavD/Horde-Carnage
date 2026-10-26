#pragma once

#include <raylib.h>

namespace horde {

// Every colour the game draws with, in one place, so the look can be changed as a whole.
inline constexpr Color kBackground{20, 18, 24, 255};
inline constexpr Color kFloor{34, 30, 40, 255};
inline constexpr Color kGrid{44, 40, 52, 255};
inline constexpr Color kWall{110, 96, 120, 255};
inline constexpr Color kPlayerFill{90, 190, 230, 255};
inline constexpr Color kPlayerEdge{20, 60, 80, 255};
inline constexpr Color kPlayerDeadFill{95, 95, 105, 255};
inline constexpr Color kPlayerDeadEdge{45, 45, 52, 255};
inline constexpr Color kHairballFill{150, 92, 70, 255};
inline constexpr Color kHairballEdge{70, 40, 32, 255};
inline constexpr Color kHairballEye{235, 225, 200, 255};
// The boss is darker and redder than its children, with eyes that glow.
inline constexpr Color kBossFill{112, 58, 64, 255};
inline constexpr Color kBossEdge{48, 24, 30, 255};
inline constexpr Color kBossEye{255, 190, 90, 255};
inline constexpr Color kBossBar{170, 32, 40, 255};
inline constexpr Color kSoftText{170, 165, 180, 255};
inline constexpr Color kBlood{220, 60, 50, 255};
inline constexpr Color kVictoryGold{230, 185, 95, 255};
inline constexpr Color kStunStar{255, 225, 130, 255};
// Paler and redder than the boss's body, so a strand reads against the dark floor and as something that
// hurts. The gap lane is the one pale, safe thing in the warning.
inline constexpr Color kStrand{225, 130, 110, 255};
inline constexpr Color kShell{95, 155, 255, 255};
inline constexpr Color kBarrier{255, 150, 50, 255};
inline constexpr Color kExposed{235, 235, 240, 255};
inline constexpr Color kRingLane{205, 205, 220, 255};
// The shockwave is dust thrown up by the landing, red where it still hurts.
inline constexpr Color kShockwave{230, 120, 95, 255};
inline constexpr Color kBossEyeEnraged{255, 60, 50, 255};
inline constexpr Color kFrenzyFill{135, 42, 50, 255};
inline constexpr Color kBarBack{50, 20, 24, 255};
inline constexpr Color kBarHurt{255, 140, 120, 255};
inline constexpr Color kBarEdge{120, 100, 110, 255};
// Light steel, so the barrel stands out against the dark floor where it leaves the body.
inline constexpr Color kGun{175, 180, 195, 255};
inline constexpr Color kMuzzleFlash{255, 220, 120, 255};
inline constexpr Color kCrosshair{240, 240, 245, 255};
inline constexpr Color kBullet{255, 230, 140, 255};
inline constexpr Color kFlask{245, 150, 45, 255};
inline constexpr Color kHitFlash{255, 255, 255, 255};

} // namespace horde
