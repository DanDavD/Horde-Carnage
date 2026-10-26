#pragma once

#include <raylib.h>

namespace horde {

// Every colour the game draws with, in one place, so the look can be changed as a whole.
//
// The crypt is cold, desaturated stone, lit only by the player's torch. Colour is kept for what means
// something: blood red for damage and danger, amber for the boss's eyes and the flasks, blue and orange for
// the two shields, warm light for the torch.
inline constexpr Color kBackground{8, 8, 11, 255};
inline constexpr Color kStone{72, 77, 88, 255};
inline constexpr Color kMortar{24, 25, 31, 255};
inline constexpr Color kCrack{34, 35, 42, 255};
inline constexpr Color kWallStone{31, 32, 38, 255};
inline constexpr Color kWallMortar{12, 12, 15, 255};
// The top edge of the walls, where the torchlight catches it.
inline constexpr Color kWallLip{92, 94, 106, 255};
inline constexpr Color kDarkness{5, 5, 9, 255};
inline constexpr Color kTorchLight{255, 168, 88, 255};
inline constexpr Color kStain{58, 14, 14, 255};
// The hunter: a torn cape and a long coat in dark leather and wool, a tricorn hat with a worn trim, a
// bandolier of brass cartridges, and gloved hands.
inline constexpr Color kCape{38, 33, 30, 255};
inline constexpr Color kCapeEdge{13, 12, 11, 255};
inline constexpr Color kCoat{74, 65, 56, 255};
inline constexpr Color kHat{30, 28, 27, 255};
inline constexpr Color kHatTrim{82, 74, 64, 255};
inline constexpr Color kHatCrown{44, 41, 39, 255};
inline constexpr Color kLeather{78, 60, 43, 255};
inline constexpr Color kBrass{204, 170, 96, 255};
inline constexpr Color kGlove{70, 52, 40, 255};
inline constexpr Color kGunWood{96, 66, 44, 255};
inline constexpr Color kGunDark{44, 46, 54, 255};
// Light steel along the top of the gun, so the barrel stands out against the dark floor.
inline constexpr Color kGun{175, 180, 195, 255};
inline constexpr Color kPlayerDeadFill{95, 95, 105, 255};
inline constexpr Color kPlayerDeadEdge{45, 45, 52, 255};
inline constexpr Color kHairballFill{150, 92, 70, 255};
inline constexpr Color kHairballEdge{70, 40, 32, 255};
inline constexpr Color kHairballEye{235, 225, 200, 255};
// The boss is darker and redder than its children, with eyes that glow.
inline constexpr Color kBossFill{112, 58, 64, 255};
inline constexpr Color kBossEdge{48, 24, 30, 255};
inline constexpr Color kBossEye{255, 190, 90, 255};
inline constexpr Color kBossBar{150, 26, 32, 255};
// Text and the frames of the HUD are the colour of old bone and parchment, not white.
inline constexpr Color kText{226, 218, 198, 255};
inline constexpr Color kSoftText{160, 154, 140, 255};
inline constexpr Color kHudAccent{206, 192, 158, 255};
inline constexpr Color kHudFrame{96, 88, 74, 255};
inline constexpr Color kBlood{200, 40, 36, 255};
inline constexpr Color kHealth{158, 30, 34, 255};
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
inline constexpr Color kBarBack{22, 14, 14, 255};
inline constexpr Color kBarHurt{228, 128, 112, 255};
inline constexpr Color kMuzzleFlash{255, 220, 120, 255};
inline constexpr Color kCrosshair{236, 228, 210, 255};
inline constexpr Color kBullet{255, 230, 140, 255};
inline constexpr Color kFlask{245, 150, 45, 255};
inline constexpr Color kHitFlash{255, 255, 255, 255};

} // namespace horde
