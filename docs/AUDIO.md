# Audio

## What it is

Every sound in the fight is made in code, like the art: a small synthesizer builds each one from a recipe when the game starts. Nothing is recorded or downloaded, so there are no sound files to ship and no licenses to track. The sounds come from the same events as the hit stop, the shake and the particles (see [Game feel](GAME_FEEL.md)), so the rules never play a sound themselves. The boss's theme is made by the same synthesizer, note by note; it has its own doc, [Music](MUSIC.md).

There are 25 sounds: two for the menus (a dry tick to move, a low knock to choose), and 23 for the fight: the two guns, the shield (a ping when a bullet bounces off the Shell, a knock when the shield soaks one up, and glass when it breaks), the boss's flesh, a hairball bursting, the dash, the flask, three hits on the player (a hit, a heavy hit and the death), the boss's landing, her crash into a wall, her roar, her collapse when the first bar runs out and her death, a strand being spat, and a warning for each of her five attacks.

The code:

- `src/core/Synth.h`, `src/core/Synth.cpp`: `SynthLayer`, `SoundRecipe` and `Synthesize`, which turns a recipe into samples.
- `src/core/SoundLibrary.h`, `src/core/SoundLibrary.cpp`: `RecipeFor`, the recipe of every sound.
- `src/core/SoundCue.h`, `src/core/SoundCue.cpp`: `SoundId`, `CueFor` (which sound an event makes, at what volume and pitch) and `Placed` (where it sits between the speakers).
- `src/app/audio/SoundBank.h`, `src/app/audio/SoundBank.cpp`: loads the sounds into raylib with a few voices each, and plays a cue.
- `src/app/Game.cpp`: opens the audio device, and plays the cue of every event after each step.

## How it works

**A recipe.** A sound is up to four layers mixed. Each layer is a waveform (sine, square, saw, triangle or noise) whose pitch slides from one frequency to another, under an envelope that waits, rises and then dies away exponentially. A layer can waver in pitch (vibrato) and run through a low-pass filter that sweeps, which is what turns white noise into a crack, a thump or a whoosh, and through a high-pass that thins out its lows before the clip (only the music's guitar uses it). The mix is pushed through a soft clip (`tanh`) for grit, scaled to a peak of 0.89, and faded out over its last 4 ms so it never ends in a click. The noise comes from a PCG32 seeded per recipe, so a sound is the same on every launch.

Most sounds are a few of the same blocks:

- **A crack or a hiss:** a burst of noise through a filter that closes. The guns, the shield, the spit.
- **Weight:** a sine that drops in pitch. A body landing, a blow, a boom.
- **A voice:** saws with a slow waver through a dark filter, over breath. The roar, the growl, the death.
- **Metal and glass:** sines that are out of tune with each other and die at different rates. The Shell's ping, the shield breaking.

**From an event to a sound.** After each step, `Game::FeelEvents` hands every event to `CueFor`, which picks the sound and its volume. The pistol and the SMG are told apart by the damage the shot event carries; a hit of 30 or more is a heavy hit; a bullet that bounces off the Shell pings, and one the shield soaks up knocks. Each cue then wanders up to 6% in pitch and 10% in volume, so the SMG's tenth shot in a second is not a copy of the first. `Placed` leans it toward the side of the screen it comes from (never more than 70% to one side) and lowers it the further off screen it is, down to half a view's width beyond the edge.

**Warnings.** Every attack now records `AttackStarted` when its warning begins, and each attack has its own sound: a growl that rises for the Charge, a breath drawn in for the Hair Ring, a grunt for the Leap, a rattle for the Volley and a howl for the Summon. A player who is looking at the bars or at a hairball still hears what is coming.

**Voices.** `LoadSoundBank` synthesizes every recipe once, loads it with `LoadSoundFromWave`, and makes up to seven more voices with `LoadSoundAlias`, which share the first one's samples. The guns get eight voices, the sounds that come several times a second five, the rest two. `PlayCue` plays the next voice in turn, so a new shot cuts off the oldest one still ringing instead of the newest.

## Design decisions

- **Synthesized, not recorded.** It fits the rest of the game, which draws everything in code, and it means no files to download, no licenses and nothing to load from disk, which also keeps the web build small. Every sound is a short recipe I can tune by changing a number.
  - Rejected: CC0 sound packs. They would sound more natural, but each file needs a source and a license noted, and they would not match each other or the procedural look. If a sound falls short in play, it can still be swapped for one.
- **Sounds come from events, like the particles.** `StepWorld` already records everything that happens. Reading the same list keeps sound out of the rules and out of the tests of the rules, and adds nothing to `horde_core` but data.
- **New events only where a sound needed one.** `AttackStarted`, `StrandsSpat` and `FlaskDrunk` had no reason to exist before: the warnings were seen on the boss, and the strands and the heal showed on their own. They neither slow the fight nor throw particles.
- **The cues are pure logic in `horde_core`.** Which sound an event makes, how loud and where, is tested without an audio device. Only loading and playing live in the app.
- **Round-robin voices instead of one sound each.** One `Sound` per sound restarts itself on every play, so the SMG would cut its own shots off ten times a second. Aliases share the samples, so eight voices of a shot cost one copy of it.
- **Their own random stream.** The pitch and volume wander comes from a generator of its own, so sound never takes a number from the fight's.
- **Full volume at 0.8.** The loudest moments mix several sounds at once, a heavy hit, a landing and a roar, and the headroom keeps them from clipping. The player turns it down from there in the settings (see [Settings](SETTINGS.md)).

## How to tune it

| What | Where | Now |
|---|---|---|
| How each sound is made | `MakeRecipes` (`SoundLibrary.cpp`) | One recipe per `SoundId` |
| How loud each event plays | `BaseCue` (`SoundCue.cpp`) | Pistol 0.7, SMG 0.5, bullet on the boss 0.4, heavy hit and boss moments 1.0 |
| How much each play wanders | `kPitchWander`, `kVolumeWander` (`SoundCue.cpp`) | 6%, 10% |
| How far a sound leans to one side | `kMaxPan` (`SoundCue.h`) | 0.7 |
| Voices per sound | `VoicesFor` (`SoundBank.cpp`), `kMaxVoices` (`SoundBank.h`) | 8 for the guns, 5 for frequent sounds, 2 for the rest |
| Overall volume | `kFullGain` (`Settings.cpp`), times the player's setting squared | 0.8 at full |

## Tests

- `tests/core/SynthTests.cpp`: a sound lasts its length, peaks at 0.89 and ends in silence; a 440 Hz sine crosses zero 440 times a second, and a slide ends on its last pitch; the envelope waits for its delay and dies away; a low-pass takes the hiss out of noise; a high-pass thins out a low tone and lets a high one through; the same recipe always makes the same samples and another seed other noise; a recipe with no length or no layers is silence; drive clips the wave and it still fits in full scale.
- `tests/core/SoundCueTests.cpp`: every event makes a sound; each gun sounds like itself and a heavy hit like a heavy hit; a bounce pings and a soaked-up bullet knocks; every attack warns with its own sound; bullets on the boss are quieter than the hits that matter; each play wanders a little and never far; every recipe makes a short sound that peaks between 0.5 and 0.9; a sound leans to the side it comes from and fades off screen.
- `tests/core/WorldEventTests.cpp`: an attack's start is recorded once, with its kind; a Hair Ring records how many strands it spat; a finished drink records the health it gave back.

I checked the rest by running the game. A temporary build wrote all 23 sounds to WAV files, which I plotted as waveforms and spectrograms to check their length, peak and shape; that showed the gunshots were mostly a low thump that laptop speakers cannot play, so I raised their crack. A temporary log in a Debug build showed the audio device starting on WASAPI and every cue of a fight playing (`IsSoundPlaying` true), with its pitch, volume and pan. In the web build, the browser's audio context was running at 48 kHz after the first click (browsers only start audio after one). I could not listen to it myself.

## Limitations

- **The sounds are as good as their recipes.** Synthesis is clean but simple; the roar and the death are the furthest from a real creature.
- **Nothing for the hairballs' spit warning, a drink starting, or casings landing.** They are quiet moments, and the events for them do not exist.
- **The hit stop does not slow the sounds.** A sound plays at its own pitch through a slow-down. Pitching everything down with the time scale would make the hits heavier, but would also make the warnings harder to recognise.
