# Art direction

## What it is

How the game looks, and how that look is made. The direction: the readable shapes of games like Brotato, but dark and close to real, in the tone of Dark Souls. The fight happens in a stone crypt lit only by the torch the player carries. Colour is kept for what means something: blood red for damage and danger, amber for the boss's eyes and the flasks, blue and orange for the two shields, warm light for the torch.

Everything is drawn in code, with no image files: the floor, the walls, the light, the hunter and the HUD. The only asset is the font, Cinzel.

It covers the arena, the player, the HUD, and the boss with her hairballs.

Where it lives:

- `src/core/StoneFloor.h`, `src/core/StoneFloor.cpp`: `LayStoneFloor`, the layout of the floor's slabs, cracks and stains. It is pure logic, so it is tested.
- `src/app/render/Art.h`, `src/app/render/Art.cpp`: `Art`, everything made once when the game starts: the two fonts, four textures made in code, and the floor's layout. `LoadArt`, `UnloadArt`, and `DrawGlow`, a soft round blot used for shadows, stains and warmth.
- `src/app/render/ArenaDraw.h`, `src/app/render/ArenaDraw.cpp`: the floor, the walls, the shadows under bodies, the torchlight and the vignette.
- `src/app/render/PlayerDraw.cpp`: the hunter.
- `src/app/render/HairBody.h`, `src/app/render/HairBody.cpp`: `DrawHairBody`, the body of matted hair the boss and the hairballs share, and `DrawHairball`.
- `src/app/render/BossDraw.cpp`: the boss's styles for each phase, and its pose in each attack: where it looks, how it is squashed or stretched, how wide its mouth is open.
- `src/app/render/Shapes.h`, `src/app/render/Shapes.cpp`: shapes raylib does not have, shared by the hunter and the hair: a closed shape from a radius at each angle, a rotated ellipse, a triangle of either winding.
- `src/app/render/Hud.cpp`, `src/app/render/EndScreen.cpp`: the HUD and the end screens, in Cinzel.
- `src/app/render/Text.h`, `src/app/render/Text.cpp`: text in the game's fonts, with letter spacing, centred or aligned to the right.
- `src/app/render/Palette.h`: every colour.
- `cmake/EmbedFile.cmake`: compiles the fonts into the executable.

## How it works

**What is made at the start.** `Game` calls `LoadArt` right after the window opens, because raylib can only make textures while a window is open, and `UnloadArt` right before it closes. `LoadArt`:

- loads Cinzel Regular at 160 pixels (titles) and Cinzel Bold at 64 pixels (the rest of the HUD) from the bytes compiled into the executable, with mipmaps;
- makes four textures pixel by pixel: a grey stone grain of 256x256 that tiles, a soft white dot, the torch's falloff (clear in the middle, opaque at the edge), and the vignette (clear in the middle, opaque in the corners);
- lays the floor with a fixed seed, so the arena is the same place in every fight.

**The floor.** `LayStoneFloor` cuts the arena into rows of the same height, as close to 80 units as divides it evenly (13 rows in 1040), and cuts each row into slabs from 90 to 220 units wide. A slab never leaves less than 90 units behind it, so a row cannot end on a sliver. Each slab gets a tone from -1 to 1, and about one in five gets a crack of three points across it. Nine stains of old blood go anywhere on the floor. `DrawFloor` draws the mortar, then each slab inset by half a joint, its tone moving the stone's colour by up to 12 per channel, with a bevel lit on the top and left edges and shaded on the others, then its crack. The grain is multiplied over the whole floor, and the stains are dark red blots.

**The walls.** `DrawWalls` darkens the floor over 40 units at the foot of every wall, then fills everything the camera can show past the arena (64 units, the camera's `wallMargin`) with two courses of near-black blocks, offset by half a block. Each block's tone comes from a hash of its position, so it is the same every frame without being stored. A pale lip runs along the top of the wall, just outside the arena, where bodies touch it.

**Shadows.** Every body gets a soft dark blot on the floor, before the light is drawn. The only light is the player's torch, so the shadows of the boss and the hairballs lean away from the player, more the further they are, up to 0.6 of their radius. The player's own shadow falls straight under them.

**The torchlight.** `DrawTorchLight` darkens the whole view with the distance from the player. The falloff texture covers a square 470 units out from the player in every direction, tinted near-black at 74%: clear for the first 18% of the reach, then fading in. Up to four rectangles fill the rest of the view with the same darkness, meeting the square's edges exactly. Then a warm glow 260 units wide is added on top. Both breathe by a few percent with two sine waves on the fight's clock, like a flame, and freeze with the world when the fight ends.

**The order.** `DrawWorld` draws the floor, the walls, the shadows and the dark first. The boss's warnings, the shockwaves, the Summon's marks, every body and every projectile come after the dark, at full brightness. Over the world, before the HUD, `DrawVignette` darkens the corners of the screen by up to 60%.

**The hunter.** The player is a monster hunter: a tricorn hat, a long coat, a torn cape, a bandolier of brass cartridges, and a gun held in both hands. `DrawPlayer` draws it from above, in layers, with every size a fraction of the body radius:

1. The cape: the round body, plus a tail that bulges out behind it (a cosine raised to the third power around one direction). The tail points behind the back when standing and behind the movement when running, turning between the two the short way round, grows by half the radius at full speed, and sways from side to side on the fight's clock. Its hem is torn into 18 teeth of uneven depth, fixed to the cape so they turn with it.
2. The coat across the shoulders, wider than deep, so the body reads as turned toward the aim.
3. The gun not in hand, where it can be seen: the SMG slung across the back, or the pistol in a holster on the right hip. Switching guns swaps them over, so the hunter shows which gun is in hand without a look at the HUD.
4. The bandolier, from the left shoulder to the right hip, with five brass cartridges.
5. The arms and the gun in hand, both hands always on it, each gun with its own hold:
   - the pistol, a revolver with its drum, held out at arm's length, the arms a triangle in front of the chest and the hands cupped together on the grip;
   - the SMG held like a rifle: the wooden stock against the chest, the right hand on the grip, the left on the foregrip, and the magazine sticking out to the left.
   Each gun ends at the muzzle the bullets leave from (`MuzzlePoint`, the same function the rules use), with a strip of light steel along it.
6. The tricorn on top of it all: three points, the front one over the face, so the hat turns with the aim; a worn trim along the brim, and the round crown in the middle.

raylib has no rotated ellipse and no free shape, so the cape, the coat and the hat are triangle fans drawn by `DrawRadialShape`, which takes the shape's distance from its centre at each angle; straight pieces (arms, guns, straps) are thick lines with a dark outline. The blink, the dash trail and the drink ring work as before. A dead hunter's colours sink toward grey and the cape stops swaying.

**The Matriarch and her hairballs.** The boss and its children are the same drawing, `DrawHairBody`, in different styles (`HairStyle`) and poses (`HairPose`). From the bottom up:

1. The outer locks: a closed shape whose distance from the centre rises at each of 24 to 42 locks of uneven length, over a few big, slow lumps, so the body is a matted clump and not a circle. Soft locks are rounded; as `bristle` grows toward 1 they come to a point. Each lock grows and shrinks a little on its own beat, and the whole body swells and shrinks by 2%, so it breathes.
2. An inner layer of shorter locks between the outer ones, for depth.
3. A sheen: a patch of lighter hair shifted toward the player's torch, so the lit side matches the shadow on the floor.
4. Short strands drawn into the hair, sweeping round the body in both directions, like tangled hair.
5. In the second phase, a mouth in front of the eyes: a dark slit with a row of teeth on each jaw, shut in a snarl, a little open in the frenzy, and wide open when it roars into the second phase, howls for a Summon, spits a Hair Ring or fires a Volley burst.
6. The eyes: slits tilted down toward the middle, which reads as a scowl, with a narrow pupil and a glow on the hair around them. Amber in the first phase, red after. The boss's eyes also cast a faint glow on the floor.

The locks stay fixed in the world while the face turns to the player, so the boss does not spin as it watches you. Squash and stretch warp every point of the body along one axis, longer one way and thinner the other so the body keeps its size: it squashes as it crouches for a Charge or a Leap, stretches along the run, stretches upward in the air, flattens on landing, and jolts toward the player on every Volley burst. The second phase bristles (32 locks, longer and sharper), and the frenzy bristles all over (42 stiff locks) and reddens. A dead boss turns grey, its locks fall flatter, and its eyes shut.

The hairballs are lighter and browner than their mother, with her slit eyes in a paler amber, a little bigger for their size so they still read at 18 units across. One about to spit swells, bristles and turns its eyes red.

The strands the boss spits are small, spiky clumps of her hair that spin as they fly, their spikes twitching. The solid ball is exactly the strand's hit circle, so what the player sees is what can hurt them; only the short spikes reach past it.

**The HUD.** Thin bars in dark frames, filled with a colour a little lighter along the top, and no numbers on them: how much is left is read from the length. Health and the dash recharge in the top left, with the flasks as small glass bottles next to them; the boss's name in Cinzel Regular over its bar at the bottom, with the shield's bar and name under and over it; the guns in the bottom right, the one in hand framed in bone; the version, the attempt and the frame rate in Cinzel. Text is the colour of old bone and parchment, not white. The crosshair has a dark outline under its pale lines, so it shows over the lit floor and the dark alike. "YOU DIED" and "VICTORY" are Cinzel Regular at 88 game pixels with wide spacing. The Debug overlay keeps raylib's own font: it is for me, not for the player.

**The font.** `cmake/Dependencies.cmake` downloads Cinzel Regular, Cinzel Bold and their license with FetchContent, from the font's own repository, pinned to one commit and checked against their SHA-256 hashes. `horde_embed_file` in `cmake/EmbedFile.cmake` turns each font into a C++ source holding its bytes as an array, written only when the bytes change, and `LoadArt` loads them from memory. The build copies the license next to the executable as `Cinzel-OFL.txt`, in both the desktop and the web build.

## Design decisions

- **Drawn in code, not painted.** The game is about readable shapes, and a shape drawn in code stays sharp at any resolution: the game is drawn at the screen's own resolution (see [Screen and fullscreen](SCREEN.md)). It also keeps the project free of art I did not make, and every look is a number I can tune.
  - Rejected for now: sprites. Mine would look worse than shapes, and the time goes to the fight. Noise textures were the next step if shapes fell short; the grain is one.
- **The slabs are shapes, the grain is a small texture that repeats.** Slab edges, bevels and cracks stay crisp at 4K. The grain is soft, so stretching it costs nothing, and at 256x256 it is cheap to make, to upload and, as a power of two, can repeat in WebGL 1.
  - Rejected: one texture of the whole floor, baked at the start. At 1x it blurs at 1.5x zoom; at 2x it is 27 MB of pixels to fill, which takes too long in a browser.
- **The floor's layout is pure logic in `horde_core`.** It is plain geometry with rules that can break in quiet ways (a gap, an overlap, a sliver at the end of a row), so it is tested like the rest of the rules. The drawing only reads it.
- **The light is an overlay of darkness, not a shader.** A texture and four rectangles, drawn with raylib's own blending, work the same on the desktop and in WebGL 1, with no shader to write twice or debug blind in a browser.
  - Rejected: a light map in a render texture. It is the way to have several lights (the boss's eyes, a muzzle flash), and it may come with the game feel pass. For one torch it is more code for the same picture.
- **A hunter in a tricorn, not a hood.** Seen from above, the head is what shows most, and a hood alone read as a monk. A tricorn, a torn cape and a bandolier read as a hunter at a glance, and the hat's front point shows where the player aims.
  - Rejected: a knight in armour. Pauldrons and a helm read well from above, but a knight is the boss-fight hero everyone draws; the hunter fits a creature hunt better.
- **Both hands on the gun, and both guns always in sight.** Each gun has its own hold, so the pistol and the SMG are told apart by the pose as well as the shape, and the gun not in hand hangs on the hunter. The guns are drawn bigger than life, as in other top-down shooters, so they read at 40 units across. For that the barrels grew from 14 and 22 units to 18 and 30, so the bullets still leave from the drawn muzzle.
- **One body of hair for the mother and her children.** The hairballs are her children, so they share her drawing at another size and in lighter colours, and every change to one shows on the other.
  - Rejected: round eyes for the hairballs. They read as cute, and the direction is never cute.
- **The mouth comes with the second phase.** In the first phase the boss is a watching mass of hair; growing a mouth full of teeth is the clearest way to say the fight has changed, next to the red eyes.
- **The locks stay put while the face turns.** Turning the whole body to face the player would make the hair spin as the player runs round it. Only the eyes and the mouth follow.
- **Squash and stretch keep the size.** Stretching one way and thinning the other keeps the body about as big as its hit circle, so the animation never makes the boss look bigger or smaller than the body the rules use.
- **Strands are spinning clumps, not streaks.** A clump of hair torn from her reads as part of the boss, and a ball shows its hit circle from every side.
  - Rejected: a round tip with a waving tail. A ring of them looked like a swarm of tadpoles.
- **Bodies and warnings are drawn after the dark.** The fight is hard and has to stay fair: a warning or a strand in the dark would be a death the player could not read. So only the floor and the walls fall into darkness, and everything that can hurt stays bright.
- **Shadows lean away from the torch.** It is cheap, and it sells the torch as the one light in the room better than any other detail.
- **Cinzel, in two weights, as static files.** It is drawn from Roman inscriptions, which suits "YOU DIED", and its lower case are small capitals, so whole sentences keep the same voice. The Bold holds up at the small sizes of the HUD, where the Regular's thin strokes fade. raylib's font loader cannot read the variable version that Google Fonts ships, so I take the static files from the font's own repository.
  - Rejected: one font for everything. The Regular at 12 pixels is too thin to read over the dark floor.
- **The font is compiled into the executable, from a pinned download.** The desktop zip stays one file, and the browser build needs no file system. FetchContent with a hash, like raylib, keeps binary files out of the repository and makes a changed file fail the build.
  - Rejected: committing the font files. It works, but every dependency of the project comes the same way, and the license travels with the build anyway.
- **Fonts rasterised large and shrunk, with mipmaps.** The HUD is laid out in game pixels and scaled to the screen. A font rasterised at the size it is drawn at would be blurred up to 1.5 times on a 1080p screen; at 160 and 64 pixels it is shrunk instead, which stays sharp up to 1440p for the titles and 4K for the rest.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Every colour | `src/app/render/Palette.h` | |
| Slab rows, widths, cracks, stains | `StoneFloorStats` (`src/core/StoneFloor.h`) | 80 rows, 90 to 220 wide, 18% cracked, 9 stains of 30 to 90 |
| Which floor | `kFloorSeed` (`src/app/render/Art.cpp`) | 1031 |
| Slab joints, tone range, bevel | `kJoint`, `kStoneToneRange`, `kBevel`, `kBevelLight`, `kBevelShade` (`ArenaDraw.cpp`) | 4, 12, 2, 12%, 30% |
| Grain strength | `kGrainDepth` (`Art.cpp`) | 20% at the darkest |
| Wall depth, blocks, shadow at the foot | `kWallDepth`, `kWallBlock`, `kWallShadowReach`, `kWallShadowStrength` (`ArenaDraw.cpp`) | 64, 96, 40, 50% |
| Torch reach, darkness, warmth, flicker | `kTorchReach`, `kDarknessStrength`, `kWarmthReach`, `kWarmth`, `kFlicker` (`ArenaDraw.cpp`), `kLitShare` (`Art.cpp`) | 470, 74%, 260, 17%, 2.5%, 18% |
| Shadow lean and strength | `kShadowLean`, `kShadowMaxLean`, `kShadowStrength` (`ArenaDraw.cpp`) | 0.08 per unit, 0.6 radii, 55% |
| Vignette | `kVignetteStrength` (`ArenaDraw.cpp`) | 60% |
| The hunter's proportions | `kCape...`, `kCoat...`, `kHat...`, `kPistol...`, `kSmg...` (`PlayerDraw.cpp`) | Fractions of the radius |
| Where the bullets leave the barrel | `WeaponStats::barrelLength` (`src/core/Weapon.h`) | 18 and 30 units |
| The boss in each phase | `kBossStyle`, `kEnragedBossStyle`, `kFrenziedBossStyle` (`BossDraw.cpp`) | 24, 32 and 42 locks; bristle 0, 0.55, 0.95 |
| Squash and stretch, and how wide the mouth opens | `kCrouchSquash`, `kRunStretch`, `kAirStretch`, `kLandingSquash`, `kBurstStretch`, `k...Mouth` (`BossDraw.cpp`) | 0.88, 1.22, 1.1, 0.84, 1.12 |
| The hairballs | `kHairballStyle` (`HairBody.cpp`) | 13 locks, eyes 1.3 times bigger |
| Breathing, lumps, sheen, eyes and teeth | `kBreath...`, `kLumpDepth`, `kSheen...`, `kEye...`, `kSlit...`, `kMouth...` (`HairBody.cpp`) | |
| Font sizes in the atlas | `kTitleFontPixels`, `kUiFontPixels` (`Art.cpp`) | 160, 64 |
| Letter spacing | `kTextTracking`, `kTitleTracking` (`Text.h`) | 6%, 14% of the size |

## Tests

- `tests/core/StoneFloorTests.cpp`: the slabs cover the whole arena exactly once (their areas add up to the arena's, and every point of a 5-unit grid lies in exactly one); every slab is between the narrowest and the widest, in rows of 80, with a tone from -1 to 1; the same seed lays the same floor and another seed another; cracks stay inside their slab and run across it from left to right, about one slab in five is cracked, and the stains are inside the arena and within their sizes; and an arena narrower than one slab gets one slab per row.

The drawing needs a window, so it is not unit tested. I checked it with window captures of a Release build, before and after, in a window of 1600x900: the start of a fight, a run into the bottom left corner (the walls, the lip, the shadow at their foot, the light following the player), the Charge's warning over the stone, the hunter with each gun, standing and running (the hold changing with the gun, the other gun slung or holstered, the cape trailing behind the run), "YOU DIED" after standing still, and, in a Debug build, the F1 overlay and "VICTORY" after F4. For the boss, before and after in the same window: the first phase standing, the Charge stretched along its run, the transformation into a roar (F3), the second phase's snarl and Volley, the frenzy (F3 again), a Hair Ring and a Leap. To see the hairballs I made the second phase open with a Summon for one Debug build, and put the line back after the captures. The web build showed the same picture in a browser, with no errors in the console. The frame rate stayed at 164 to 165 on my 165 Hz monitor.

## Limitations

- **The torch is the only light that matters.** The boss's eyes only add a faint glow to the floor, and a muzzle flash does not light the room.
- **Every hairball has the same locks.** They differ only in where they look. Giving each its own pattern would need a number that stays with it, and the pool moves hairballs around as others die.
- **Strands vanish at the end of their range.** They do not fall to the floor.
- **The light does not touch the bodies.** It is on purpose (see above), but a body in a dark corner looks lit from nowhere.
- **The HUD can cover the player at the south wall.** The camera stops 64 units past the wall, and the boss's bar and the controls line sit over the bottom of the screen, as before the art pass.
- **Captures of a window that fills the whole monitor show only its first frame,** as with fullscreen. The captures come from a window resized below the screen's size.
- **The titles soften above 1440p.** They are rasterised at 160 pixels and grow past that on a 4K screen.
